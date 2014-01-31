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

  if ( configuration_->dropEventData ) return;

  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(6) << runNumber_;

  boost::filesystem::path rawRunDir( configuration_->rawDataDir.value_ );
  rawRunDir /= runDir.str();
  runRawDataDir_ = rawRunDir / "open";
  createDir(runRawDataDir_);

  runMetaDataDir_ = configuration_->metaDataDir.value_;
  runMetaDataDir_ /= runDir.str();
  createDir(runMetaDataDir_);

  boost::filesystem::path streamFileName = runRawDataDir_ / "stream";
  for (uint16_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    std::ostringstream fileName;
    fileName << streamFileName.string() << std::hex << i;
    StreamHandlerPtr streamHandler( new StreamHandler(fileName.str(),configuration_) );
    streamHandlers_.insert( StreamHandlers::value_type(i,streamHandler) );
  }

  getHLTmenu(rawRunDir);
  createLockFile(rawRunDir);

  const boost::filesystem::path jsdDir = runMetaDataDir_ / configuration_->jsdDirName.value_;
  createDir(jsdDir);
  defineRawData(jsdDir);
  defineEoLS(jsdDir);
  defineEoR(jsdDir);

  doProcessing_ = true;
  fileMoverWorkLoop_->submit(fileMoverAction_);
  lumiAccountingWorkLoop_->submit(lumiAccountingAction_);
}


void evb::bu::DiskWriter::drain()
{}


void evb::bu::DiskWriter::stopProcessing()
{
  if ( configuration_->dropEventData ) return;

  while ( lumiAccountingActive_ || fileMoverActive_ ) ::usleep(1000);
  doProcessing_ = false;

  for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->closeFile();
  }

  moveFiles();
  doLumiSectionAccounting();

  for (LumiStatistics::const_iterator it = lumiStatistics_.begin(), itEnd = lumiStatistics_.end();
       it != itEnd; ++it)
  {
    writeEoLS(it->second);

    if ( it->second->nbEvents > 0 )
      ++diskWriterMonitoring_.nbLumiSections;
  }
  lumiStatistics_.clear();

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
    doLumiSectionAccounting();
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


void evb::bu::DiskWriter::doLumiSectionAccounting()
{
  ResourceManager::LumiSectionAccountPtr lumiSectionAccount;

  while ( resourceManager_->getNextLumiSectionAccount(lumiSectionAccount) )
  {
    const uint32_t totalEventsInLumiSection =
      ruProxy_->getTotalEventsInLumiSection(lumiSectionAccount->lumiSection);

    if ( lumiSectionAccount->nbEvents == 0 )
    {
      // empty lumi section
      const LumiInfoPtr lumiInfo( new LumiInfo(lumiSectionAccount->lumiSection) );
      lumiInfo->totalEvents = totalEventsInLumiSection;
      diskWriterMonitoring_.currentLumiSection = lumiSectionAccount->lumiSection;
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
      pos->second->totalEvents = totalEventsInLumiSection;
    }
  }

  LumiStatistics::iterator it = lumiStatistics_.begin();
  while ( it != lumiStatistics_.end() )
  {
    if ( it->second->nbEvents > 0 && it->second->nbEvents == it->second->nbEventsWritten )
    {
      writeEoLS(it->second);

      if ( it->second->nbEvents > 0 )
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
  StreamHandler::FileStatisticsPtr fileStatistics;
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


void evb::bu::DiskWriter::handleRawDataFile(const StreamHandler::FileStatisticsPtr& fileStatistics)
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

  items.add("nbFilesWritten", &nbFilesWritten_);
  items.add("nbLumiSections", &nbLumiSections_);
}


void evb::bu::DiskWriter::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  nbFilesWritten_ = diskWriterMonitoring_.nbFiles;
  nbLumiSections_ = diskWriterMonitoring_.nbLumiSections;
}


void evb::bu::DiskWriter::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  diskWriterMonitoring_.nbFiles = 0;
  diskWriterMonitoring_.nbEventsWritten = 0;
  diskWriterMonitoring_.nbLumiSections = 0;
  diskWriterMonitoring_.lastEventNumberWritten = 0;
  diskWriterMonitoring_.currentLumiSection = 0;
}


void evb::bu::DiskWriter::configure()
{
  streamHandlers_.clear();
  lumiStatistics_.clear();

  if ( configuration_->dropEventData ) return;

  createDir(configuration_->rawDataDir.value_);
  DiskUsagePtr rawDiskUsage(
    new DiskUsage(configuration_->rawDataDir.value_,configuration_->rawDataLowWaterMark,configuration_->rawDataHighWaterMark,configuration_->deleteRawDataFiles)
  );
  resourceManager_->monitorDiskUsage(rawDiskUsage);

  if ( configuration_->metaDataDir != configuration_->rawDataDir )
  {
    createDir(configuration_->metaDataDir.value_);
    DiskUsagePtr metaDiskUsage(
      new DiskUsage(configuration_->metaDataDir.value_,configuration_->metaDataLowWaterMark,configuration_->metaDataHighWaterMark,false)
    );
    resourceManager_->monitorDiskUsage(metaDiskUsage);
  }

  resetMonitoringCounters();
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


void evb::bu::DiskWriter::printHtml(xgi::Output *out) const
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>DiskWriter</p>"                                     << std::endl;
  *out << "<table>"                                               << std::endl;

  {
    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number written</td>"                      << std::endl;
    *out << "<td>" << diskWriterMonitoring_.lastEventNumberWritten  << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># files written</td>"                              << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbFiles << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events written</td>"                             << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbEventsWritten << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># lumi sections with files</td>"                   << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbLumiSections << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>current lumi section</td>"                         << std::endl;
    *out << "<td>" << diskWriterMonitoring_.currentLumiSection << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void evb::bu::DiskWriter::closeAnyOldRuns() const
{
  const boost::filesystem::path rawDataDir( configuration_->rawDataDir.value_ );

  if ( ! boost::filesystem::exists(rawDataDir) ) return;

  boost::filesystem::directory_iterator dirIter(rawDataDir);

  while ( dirIter != boost::filesystem::directory_iterator() )
  {
    const std::string fileName = dirIter->filename();
    size_t pos = fileName.rfind("run");
    if ( pos != std::string::npos )
    {
      boost::filesystem::path eolsPath = *dirIter;
      eolsPath /= boost::filesystem::path( "EoR_" + fileName.substr(pos+3) + ".jsn" );
      if ( ! boost::filesystem::exists(eolsPath) )
      {
        std::ofstream json(eolsPath.string().c_str());
        json.close();
      }
    }
    ++dirIter;
  }
}


void evb::bu::DiskWriter::getHLTmenu(const boost::filesystem::path& runDir) const
{
  const boost::filesystem::path hltPath( runDir / configuration_->hltDirName.value_ );
  createDir(hltPath);

  std::string url(configuration_->hltParameterSetURL.value_);

  if ( url.empty() )
  {
    XCEPT_RAISE(exception::Configuration, "hltParameterSetURL for HLT menu not set");
  }

  CURL* curl = curl_easy_init();
  if ( ! curl )
  {
    XCEPT_RAISE(exception::DiskWriting, "Could not initialize curl for retrieving the HLT menu");
  }

  try
  {
    char lastChar = *url.rbegin();
    if ( lastChar != '/' ) url += '/';

    retrieveFromURL(curl, url+"HltConfig.py", hltPath/"HltConfig.py");
    retrieveFromURL(curl, url+"SCRAM_ARCH", hltPath/"SCRAM_ARCH");
    retrieveFromURL(curl, url+"CMSSW_VERSION", hltPath/"CMSSW_VERSION");
  }
  catch(xcept::Exception& e)
  {
    curl_easy_cleanup(curl);
    throw(e);
  }

  curl_easy_cleanup(curl);
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
    "EoLS_"  << std::setw(4) << lumiInfo->lumiSection <<
    ".jsn";
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
  json << "   \"data\" : [ \""     << lumiInfo->nbEventsWritten << "\", \""
                                   << lumiInfo->fileCount << "\", \""
                                   << lumiInfo->totalEvents << "\" ],"     << std::endl;
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
    "EoR_" << std::setw(6) << runNumber_ <<
    ".jsn";
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
  json << "   \"data\" : [ \""     << diskWriterMonitoring_.nbEventsWritten << "\", \""
                                   << diskWriterMonitoring_.nbFiles         << "\", \""
                                   << diskWriterMonitoring_.nbLumiSections  << "\" ],"  << std::endl;
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
  json << "         \"operation\" : \"sum\""                  << std::endl;
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
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"TotalEvents\","              << std::endl;
  json << "         \"operation\" : \"max\""                  << std::endl;
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
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NLumis\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
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
