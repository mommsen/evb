#include <errno.h>
#include <iomanip>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/RUproxy.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xdata/String.h"
#include "xdata/Vector.h"


evb::bu::DiskWriter::DiskWriter
(
  BU* bu,
  boost::shared_ptr<ResourceManager> resourceManager
) :
  bu_(bu),
  resourceManager_(resourceManager),
  configuration_(bu->getConfiguration()),
  buInstance_(bu->getApplicationDescriptor()->getInstance()),
  doProcessing_(false),
  lumiAccountingActive_(false),
  fileMoverActive_(false)
{
  resetMonitoringCounters();
  startLumiAccounting();
  startFileMover();
  umask(0);
}


evb::bu::StreamHandlerPtr evb::bu::DiskWriter::getStreamHandler(const uint16_t builderId) const
{
  return streamHandlers_.at(builderId);
}


void evb::bu::DiskWriter::startProcessing(const uint32_t runNumber)
{
  closeAnyOldRuns();
  resetMonitoringCounters();
  runNumber_ = runNumber;

  doProcessing_ = true;
  lumiAccountingWorkLoop_->submit(lumiAccountingAction_);

  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(6) << runNumber_;

  boost::filesystem::path rawRunDir( configuration_->rawDataDir.value_ );
  rawRunDir /= runDir.str();
  runRawDataDir_ = rawRunDir / "open";

  runMetaDataDir_ = configuration_->metaDataDir.value_;
  runMetaDataDir_ /= runDir.str();

  boost::filesystem::path streamFileName = runRawDataDir_ / "stream";
  streamHandlers_.clear();
  for (uint16_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    std::ostringstream fileName;
    fileName << streamFileName.string() << std::hex << i;
    StreamHandlerPtr streamHandler( new StreamHandler(bu_,fileName.str()) );
    streamHandlers_.insert( StreamHandlers::value_type(i,streamHandler) );
  }

  if ( ! configuration_->dropEventData )
  {
    createDir(runRawDataDir_);
    createDir(runMetaDataDir_);

    getHLTmenu(rawRunDir);
    createLockFile(rawRunDir);

    const boost::filesystem::path jsdDir = runMetaDataDir_ / configuration_->jsdDirName.value_;
    createDir(jsdDir);
    defineRawData(jsdDir);
    defineEoLS(jsdDir);
    defineEoR(jsdDir);

    fileMoverWorkLoop_->submit(fileMoverAction_);
  }
}


void evb::bu::DiskWriter::drain()
{}


void evb::bu::DiskWriter::stopProcessing()
{
  doProcessing_ = false;
  while ( lumiAccountingActive_ || fileMoverActive_ ) ::usleep(1000);

  if ( configuration_->dropEventData )
  {
    streamHandlers_.clear();
    return;
  }

  for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->closeFile();
  }

  moveFiles();
  doLumiSectionAccounting(false);

  if ( ! lumiStatistics_.empty() )
  {
    std::ostringstream oss;
    oss << "There are unaccounted files for the following lumi sections:";
    for (LumiStatistics::iterator it = lumiStatistics_.begin(), itEnd = lumiStatistics_.end();
         it != itEnd; ++it)
    {
      oss << " " << it->first;
    }
    XCEPT_RAISE(exception::DiskWriting, oss.str());

    lumiStatistics_.clear();
  }

  writeEoR();

  streamHandlers_.clear();
  removeDir(runRawDataDir_);
}


void evb::bu::DiskWriter::startLumiAccounting()
{
  try
  {
    lumiAccountingWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(bu_->getIdentifier("lumiAccounting"), "waiting");

    if ( !lumiAccountingWorkLoop_->isActive() )
      lumiAccountingWorkLoop_->activate();

    lumiAccountingAction_ =
      toolbox::task::bind(this,
                          &evb::bu::DiskWriter::lumiAccounting,
                          bu_->getIdentifier("lumiAccountingAction"));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start lumi accounting workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::DiskWriter::lumiAccounting(toolbox::task::WorkLoop* wl)
{
  lumiAccountingActive_ = true;

  try
  {
    doLumiSectionAccounting(true);
  }
  catch(xcept::Exception& e)
  {
    lumiAccountingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    lumiAccountingActive_ = false;
    XCEPT_DECLARE(exception::DiskWriting,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    lumiAccountingActive_ = false;
    XCEPT_DECLARE(exception::DiskWriting,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  lumiAccountingActive_ = false;

  ::usleep(1000);

  return doProcessing_;
}


void evb::bu::DiskWriter::doLumiSectionAccounting(const bool completeLumiSectionsOnly)
{
  ResourceManager::LumiSectionAccountPtr lumiSectionAccount;

  while ( resourceManager_->getNextLumiSectionAccount(lumiSectionAccount,completeLumiSectionsOnly) )
  {
    if ( configuration_->dropEventData ) continue;

    const uint32_t totalEventsInLumiSection =
      bu_->getRUproxy()->getTotalEventsInLumiSection(lumiSectionAccount->lumiSection);

    if ( lumiSectionAccount->nbEvents == 0 )
    {
      // empty lumi section
      const LumiInfoPtr lumiInfo( new LumiInfo(lumiSectionAccount->lumiSection) );
      lumiInfo->totalEvents = totalEventsInLumiSection;
      diskWriterMonitoring_.currentLumiSection = lumiSectionAccount->lumiSection + 1;
      writeEoLS(lumiInfo);
    }
    else
    {
      LumiStatistics::iterator pos = getLumiStatistics(lumiSectionAccount->lumiSection);
      if ( pos->second->nbEvents > 0 )
      {
        std::ostringstream oss;
        oss << "Got a duplicated account for lumi section " << lumiSectionAccount->lumiSection;
        XCEPT_RAISE(exception::EventOrder, oss.str());
      }
      pos->second->nbEvents = lumiSectionAccount->nbEvents;
      pos->second->nbIncompleteEvents = lumiSectionAccount->nbIncompleteEvents;
      pos->second->totalEvents = totalEventsInLumiSection;
    }
  }

  LumiStatistics::iterator it = lumiStatistics_.begin();
  while ( it != lumiStatistics_.end() )
  {
    if ( it->second->nbEvents > 0 &&
         it->second->nbEvents == it->second->nbEventsWritten + it->second->nbIncompleteEvents )
    {
      writeEoLS(it->second);

      if ( it->second->nbEventsWritten > 0 )
        ++diskWriterMonitoring_.nbLumiSections;

      lumiStatistics_.erase(it++);
    }
    else
    {
      ++it;
    }
  }
}


void evb::bu::DiskWriter::startFileMover()
{
  try
  {
    fileMoverWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(bu_->getIdentifier("fileMover"), "waiting");

    if ( !fileMoverWorkLoop_->isActive() )
      fileMoverWorkLoop_->activate();

    fileMoverAction_ =
      toolbox::task::bind(this,
                          &evb::bu::DiskWriter::fileMover,
                          bu_->getIdentifier("fileMoverAction"));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start file mover workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::DiskWriter::fileMover(toolbox::task::WorkLoop* wl)
{
  fileMoverActive_ = true;

  try
  {
    moveFiles();
  }
  catch(xcept::Exception& e)
  {
    fileMoverActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    fileMoverActive_ = false;
    XCEPT_DECLARE(exception::DiskWriting,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    fileMoverActive_ = false;
    XCEPT_DECLARE(exception::DiskWriting,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  fileMoverActive_ = false;

  ::usleep(1000);

  return doProcessing_;
}


void evb::bu::DiskWriter::moveFiles()
{
  FileStatisticsPtr fileStatistics;
  bool workDone;

  do
  {
    workDone = false;
    const time_t oldLumiSectionTime = time(0) - configuration_->lumiSectionTimeout;

    for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
         it != itEnd; ++it)
    {
      while ( it->second->getFileStatistics(fileStatistics) )
      {
        workDone = true;
        handleRawDataFile(fileStatistics);
      }
      workDone |= it->second->closeFileIfOpenedBefore(oldLumiSectionTime);
    }
  } while ( workDone );
}


void evb::bu::DiskWriter::handleRawDataFile(const FileStatisticsPtr& fileStatistics)
{
  const LumiStatistics::iterator lumiStatistics = getLumiStatistics(fileStatistics->lumiSection);

  std::ostringstream fileNameStream;
  fileNameStream << std::setfill('0') <<
    "run"<< std::setw(6) << runNumber_ <<
    "_ls" << std::setw(4) << fileStatistics->lumiSection <<
    "_index" << std::setw(6) << lumiStatistics->second->index++ <<
    ".raw";
  //boost::filesystem::remove(runRawDataDir_ / fileStatistics->fileName);
  const boost::filesystem::path destination( runRawDataDir_.parent_path() / fileNameStream.str() );
  boost::filesystem::rename(fileStatistics->fileName, destination);

  boost::filesystem::path jsonFile( runMetaDataDir_ / fileNameStream.str() );
  jsonFile.replace_extension("jsn");
  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  const std::string path = jsonFile.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                                     << std::endl;
  json << "   \"data\" : [ \""     << fileStatistics->nbEventsWritten << "\" ],"  << std::endl;
  json << "   \"definition\" : \"" << rawDataDefFile_.string() << "\","           << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_  << "\""                        << std::endl;
  json << "}"                                                                     << std::endl;
  json.close();

  boost::filesystem::rename(path,jsonFile);

  {
    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

    ++diskWriterMonitoring_.nbFiles;
    diskWriterMonitoring_.nbEventsWritten += fileStatistics->nbEventsWritten;
    if ( diskWriterMonitoring_.lastEventNumberWritten < fileStatistics->lastEventNumberWritten )
      diskWriterMonitoring_.lastEventNumberWritten = fileStatistics->lastEventNumberWritten;
    if ( diskWriterMonitoring_.currentLumiSection < fileStatistics->lumiSection )
      diskWriterMonitoring_.currentLumiSection = fileStatistics->lumiSection;
    if ( diskWriterMonitoring_.lastLumiSection < fileStatistics->lumiSection )
      diskWriterMonitoring_.lastLumiSection = fileStatistics->lumiSection;
  }

  ++(lumiStatistics->second->fileCount);
  lumiStatistics->second->nbEventsWritten += fileStatistics->nbEventsWritten;
}


evb::bu::DiskWriter::LumiStatistics::iterator evb::bu::DiskWriter::getLumiStatistics(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(lumiStatisticsMutex_);

  LumiStatistics::iterator pos = lumiStatistics_.lower_bound(lumiSection);
  if ( pos == lumiStatistics_.end() || lumiStatistics_.key_comp()(lumiSection,pos->first) )
  {
    // new lumi section
    const LumiInfoPtr lumiInfo( new LumiInfo(lumiSection) );
    pos = lumiStatistics_.insert(pos, LumiStatistics::value_type(lumiSection,lumiInfo));
  }
  return pos;
}


void evb::bu::DiskWriter::appendMonitoringItems(InfoSpaceItems& items)
{
  nbFilesWritten_ = 0;
  nbLumiSections_ = 0;
  currentLumiSection_ = 0;

  items.add("nbFilesWritten", &nbFilesWritten_);
  items.add("nbLumiSections", &nbLumiSections_);
  items.add("currentLumiSection", &currentLumiSection_);
}


void evb::bu::DiskWriter::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  nbFilesWritten_ = diskWriterMonitoring_.nbFiles;
  nbLumiSections_ = diskWriterMonitoring_.nbLumiSections;
  currentLumiSection_ = diskWriterMonitoring_.currentLumiSection;
}


void evb::bu::DiskWriter::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  diskWriterMonitoring_.nbFiles = 0;
  diskWriterMonitoring_.nbEventsWritten = 0;
  diskWriterMonitoring_.nbLumiSections = 0;
  diskWriterMonitoring_.lastEventNumberWritten = 0;
  diskWriterMonitoring_.currentLumiSection = 0;
  diskWriterMonitoring_.lastLumiSection = 0;
}


void evb::bu::DiskWriter::configure()
{
  streamHandlers_.clear();
  lumiStatistics_.clear();

  resetMonitoringCounters();

  if ( ! configuration_->dropEventData )
  {
    createDir(configuration_->rawDataDir.value_);
    createDir(configuration_->metaDataDir.value_);
  }
}


void evb::bu::DiskWriter::createDir(const boost::filesystem::path& path) const
{
  if ( ! boost::filesystem::exists(path) &&
       ( ! boost::filesystem::create_directories(path) ) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << path.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}


void evb::bu::DiskWriter::removeDir(const boost::filesystem::path& path) const
{
  if ( boost::filesystem::exists(path) &&
       ( ! boost::filesystem::remove(path) ) )
  {
    std::ostringstream oss;
    oss << "Failed to remove directory " << path.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}


cgicc::div evb::bu::DiskWriter::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("DiskWriter"));

  {
    table table;
    table.set("title","Statistics for files generated. This numbers are updated only then a file is closed. Thus, they lag a bit behind. If 'dropEventData' is true, these counters stay at 0.");

    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

    table.add(tr()
              .add(td("last evt number written"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.lastEventNumberWritten))));
    table.add(tr()
              .add(td("# files written"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.nbFiles))));
    table.add(tr()
              .add(td("# events written"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.nbEventsWritten))));
    table.add(tr()
              .add(td("# finished lumi sections with files"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.nbLumiSections))));
    table.add(tr()
              .add(td("last lumi section with files"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.lastLumiSection))));
    table.add(tr()
              .add(td("current lumi section"))
              .add(td(boost::lexical_cast<std::string>(diskWriterMonitoring_.currentLumiSection))));
    div.add(table);
  }

  return div;
}


void evb::bu::DiskWriter::closeAnyOldRuns() const
{
  const boost::filesystem::path rawDataDir( configuration_->rawDataDir.value_ );

  if ( ! boost::filesystem::exists(rawDataDir) ) return;

  boost::filesystem::directory_iterator dirIter(rawDataDir);

  while ( dirIter != boost::filesystem::directory_iterator() )
  {
    const std::string fileName = dirIter->path().string();
    size_t pos = fileName.rfind("run");
    if ( pos != std::string::npos )
    {
      boost::filesystem::path eorPath = *dirIter;
      eorPath /= boost::filesystem::path( "run" + fileName.substr(pos+3) + "_ls0000_EoR.jsn" );
      if ( ! boost::filesystem::exists(eorPath) )
      {
        try
        {
          std::ofstream json(eorPath.string().c_str());
          json.close();
        }
        catch (...) {} // Ignore any failures in case that the run directory is removed while we try to write
      }
    }
    ++dirIter;
  }
}


void evb::bu::DiskWriter::getHLTmenu(const boost::filesystem::path& runDir) const
{
  if ( configuration_->hltParameterSetURL.value_.empty() ) return;

  const boost::filesystem::path tmpPath = runDir / "curl";
  createDir(tmpPath);

  std::string url(configuration_->hltParameterSetURL.value_);

  CURL* curl = curl_easy_init();
  if ( ! curl )
  {
    XCEPT_RAISE(exception::DiskWriting, "Could not initialize curl for retrieving the HLT menu");
  }

  try
  {
    char lastChar = *url.rbegin();
    if ( lastChar != '/' ) url += '/';

    for (xdata::Vector<xdata::String>::iterator it = configuration_->hltFiles.begin(), itEnd = configuration_->hltFiles.end();
         it != itEnd; ++it)
    {
      const std::string& fileName = it->toString();
      retrieveFromURL(curl, url+fileName, tmpPath/fileName);
    }
  }
  catch(xcept::Exception& e)
  {
    curl_easy_cleanup(curl);
    throw(e);
  }

  curl_easy_cleanup(curl);

  const boost::filesystem::path hltPath( runDir / configuration_->hltDirName.value_  );
  boost::filesystem::rename(tmpPath,hltPath);
}


void evb::bu::DiskWriter::retrieveFromURL(CURL* curl, const std::string& url, const boost::filesystem::path& output) const
{
  const char* path = output.string().c_str();
  FILE* file = fopen(path,"w");
  if ( ! file )
  {
    std::ostringstream msg;
    msg << "Failed to open file " << path
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, msg.str());
  }

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //allow libcurl to follow redirection
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

  const CURLcode result = curl_easy_perform(curl);

  if ( result != CURLE_OK )
  {
    fclose(file);

    std::ostringstream msg;
    msg << "Failed to retrieve the HLT information from " << url
      << ": " << curl_easy_strerror(result);
    XCEPT_RAISE(exception::DiskWriting, msg.str());
  }

  fclose(file);
}


void evb::bu::DiskWriter::createLockFile(const boost::filesystem::path& runDir) const
{
  const boost::filesystem::path fulockPath( runDir / configuration_->fuLockName.value_ );
  const char* path = fulockPath.string().c_str();
  std::ofstream fulock(path);
  fulock << "1 0";
  fulock.close();
}


void evb::bu::DiskWriter::writeEoLS(const LumiInfoPtr& lumiInfo) const
{
  std::ostringstream fileNameStream;
  fileNameStream << std::setfill('0') <<
    "run" << std::setw(6) << runNumber_ <<
    "_ls"  << std::setw(4) << lumiInfo->lumiSection <<
    "_EoLS.jsn";
  const boost::filesystem::path jsonFile = runMetaDataDir_ / fileNameStream.str();

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  const std::string path = jsonFile.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                              << std::endl;
  json << "   \"data\" : [ \""
    << lumiInfo->nbEventsWritten << "\", \""
    << lumiInfo->fileCount << "\", \""
    << lumiInfo->totalEvents << "\", \""
    << lumiInfo->nbIncompleteEvents << "\" ],"                             << std::endl;
  json << "   \"definition\" : \"" << eolsDefFile_.string() << "\","       << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_ << "\""                  << std::endl;
  json << "}"                                                              << std::endl;
  json.close();

  boost::filesystem::rename(path,jsonFile);
}


void evb::bu::DiskWriter::writeEoR() const
{
  std::ostringstream fileNameStream;
  fileNameStream << std::setfill('0') <<
    "run" << std::setw(6) << runNumber_ <<
    "_ls0000_EoR.jsn";
  const boost::filesystem::path jsonFile = runMetaDataDir_ / fileNameStream.str();

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
  const std::string path = jsonFile.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                                           << std::endl;
  json << "   \"data\" : [ \""
    << diskWriterMonitoring_.nbEventsWritten << "\", \""
    << diskWriterMonitoring_.nbFiles         << "\", \""
    << diskWriterMonitoring_.nbLumiSections  << "\", \""
    << diskWriterMonitoring_.lastLumiSection << "\" ],"  << std::endl;
  json << "   \"definition\" : \"" << eorDefFile_.string() << "\","                     << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""                             << std::endl;
  json << "}"                                                                           << std::endl;
  json.close();

  boost::filesystem::rename(path,jsonFile);
}


void evb::bu::DiskWriter::defineRawData(const boost::filesystem::path& jsdDir)
{
  rawDataDefFile_ = jsdDir / "rawData.jsd";
  const std::string path = rawDataDefFile_.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ]"                                              << std::endl;
  json << "}"                                                 << std::endl;
  json.close();

  boost::filesystem::rename(path,rawDataDefFile_);
}


void evb::bu::DiskWriter::defineEoLS(const boost::filesystem::path& jsdDir)
{
  eolsDefFile_ = jsdDir / "EoLS.jsd";

  const std::string path = eolsDefFile_.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"TotalEvents\","              << std::endl;
  json << "         \"operation\" : \"max\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NLostEvents\","              << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ]"                                              << std::endl;
  json << "}"                                                 << std::endl;
  json.close();

  boost::filesystem::rename(path,eolsDefFile_);
}


void evb::bu::DiskWriter::defineEoR(const boost::filesystem::path& jsdDir)
{
  eorDefFile_ = jsdDir / "EoR.jsd";

  const std::string path = eorDefFile_.string() + ".tmp";
  std::ofstream json(path.c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NLumis\","                   << std::endl;
  json << "         \"operation\" : \"max\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"LastLumi\","                 << std::endl;
  json << "         \"operation\" : \"max\","                 << std::endl;
  json << "         \"type\" : \"integer\""                   << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ]"                                              << std::endl;
  json << "}"                                                 << std::endl;
  json.close();

  boost::filesystem::rename(path,eorDefFile_);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
