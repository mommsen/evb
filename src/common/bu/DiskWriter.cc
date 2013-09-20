#include <sstream>
#include <iomanip>
#include <sys/stat.h>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/ResourceManager.h"
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
buInstance_( bu->getApplicationDescriptor()->getInstance() ),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
  startLumiMonitoring();
}


evb::bu::StreamHandlerPtr evb::bu::DiskWriter::getStreamHandler(const uint16_t builderId) const
{
  return streamHandlers_.at(builderId);
}


void evb::bu::DiskWriter::startProcessing(const uint32_t runNumber)
{
  if ( configuration_->dropEventData ) return;

  resetMonitoringCounters();

  runNumber_ = runNumber;

  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(6) << runNumber_;

  boost::filesystem::path rawRunDir( configuration_->rawDataDir.value_ );
  rawRunDir /= runDir.str();
  runRawDataDir_ = rawRunDir / "open";
  DiskWriter::createDir(runRawDataDir_);

  runMetaDataDir_ = configuration_->metaDataDir.value_;
  runMetaDataDir_ /= runDir.str();
  DiskWriter::createDir(runMetaDataDir_);

  for (uint16_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    StreamHandlerPtr streamHandler(
      new StreamHandler(buInstance_,runNumber_,runRawDataDir_,runMetaDataDir_,configuration_)
    );
    streamHandlers_.insert( StreamHandlers::value_type(i,streamHandler) );
  }

  createLockFile(rawRunDir);
  defineEoLSjson();
  defineEoRjson();

  doProcessing_ = true;
  lumiMonitoringWorkLoop_->submit(lumiMonitoringAction_);
}


void evb::bu::DiskWriter::drain()
{}


void evb::bu::DiskWriter::stopProcessing()
{
  if ( configuration_->dropEventData ) return;

  while ( processActive_ ) ::usleep(1000);
  doProcessing_ = false;

  for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->close();
  }

  {
    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
    gatherLumiStatistics();
    writeEoR();
  }

  streamHandlers_.clear();
  removeDir(runRawDataDir_);
}


void evb::bu::DiskWriter::startLumiMonitoring()
{
  try
  {
    lumiMonitoringWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(bu_->getIdentifier("lumiMonitoring"), "waiting");

    if ( !lumiMonitoringWorkLoop_->isActive() )
      lumiMonitoringWorkLoop_->activate();

    lumiMonitoringAction_ =
      toolbox::task::bind(this,
        &evb::bu::DiskWriter::updateLumiMonitoring,
        bu_->getIdentifier("lumiMonitoringAction"));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start lumi monitoring workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::DiskWriter::updateLumiMonitoring(toolbox::task::WorkLoop* wl)
{
  processActive_ = true;

  try
  {
    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
    gatherLumiStatistics();
  }
  catch(xcept::Exception& e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    processActive_ = false;
    XCEPT_DECLARE(exception::L1Trigger,
      sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    processActive_ = false;
    XCEPT_DECLARE(exception::L1Trigger,
      sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  processActive_ = false;

  ::sleep(5);

  return doProcessing_;
}


void evb::bu::DiskWriter::gatherLumiStatistics()
{
  LumiMonitorPtr lumiMonitor;
  std::pair<LumiMonitors::iterator,bool> result;
  const uint32_t streamCount = streamHandlers_.size();
  bool workDone;

  do
  {
    workDone = false;

    for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
         it != itEnd; ++it)
    {
      while ( it->second->getLumiMonitor(lumiMonitor) )
      {
        workDone = true;
        result = lumiMonitors_.insert(lumiMonitor);

        if ( result.second == false)        // lumisection exists
          *(*result.first) += *lumiMonitor; // add values and see the stars

        diskWriterMonitoring_.nbFiles += lumiMonitor->nbFiles;
        diskWriterMonitoring_.nbEventsWritten += lumiMonitor->nbEventsWritten;
        if ( diskWriterMonitoring_.lastEventNumberWritten < lumiMonitor->lastEventNumberWritten )
          diskWriterMonitoring_.lastEventNumberWritten = lumiMonitor->lastEventNumberWritten;
        if ( diskWriterMonitoring_.currentLumiSection < lumiMonitor->lumiSection )
          diskWriterMonitoring_.currentLumiSection = lumiMonitor->lumiSection;

        if ( (*result.first)->updates == streamCount )
        {
          if ( (*result.first)->nbFiles > 0 )
            ++diskWriterMonitoring_.nbLumiSections;

          writeEoLS( (*result.first)->lumiSection, (*result.first)->nbFiles, (*result.first)->nbEventsWritten );
          FileHandler::removeIndexForLumiSection( (*result.first)->lumiSection );
          lumiMonitors_.erase(result.first);
        }
      }
    }
  } while ( workDone );
}


void evb::bu::DiskWriter::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEvtsWritten_ = 0;
  nbFilesWritten_ = 0;

  items.add("nbEvtsWritten", &nbEvtsWritten_);
  items.add("nbFilesWritten", &nbFilesWritten_);
}


void evb::bu::DiskWriter::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  nbEvtsWritten_ = diskWriterMonitoring_.nbEventsWritten;
  nbFilesWritten_ = diskWriterMonitoring_.nbFiles;
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
  lumiMonitors_.clear();

  if ( configuration_->dropEventData ) return;

  createDir(configuration_->rawDataDir.value_);
  if ( configuration_->rawDataLowWaterMark > configuration_->rawDataHighWaterMark )
  {
    std::ostringstream oss;
    oss << "The high water mark " << configuration_->rawDataHighWaterMark.value_;
    oss << " for the raw data path " << configuration_->rawDataDir.value_;
    oss << " is smaller than the low water mark " << configuration_->rawDataLowWaterMark.value_;
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  DiskUsagePtr rawDiskUsage(
    new DiskUsage(configuration_->rawDataDir.value_,configuration_->rawDataLowWaterMark,configuration_->rawDataHighWaterMark,configuration_->deleteRawDataFiles)
  );
  resourceManager_->monitorDiskUsage(rawDiskUsage);

  createDir(configuration_->metaDataDir.value_);
  if ( configuration_->metaDataLowWaterMark > configuration_->metaDataHighWaterMark )
  {
    std::ostringstream oss;
    oss << "The high water mark " << configuration_->metaDataHighWaterMark.value_;
    oss << " for the meta data path " << configuration_->metaDataDir.value_;
    oss << " is smaller than the low water mark " << configuration_->metaDataLowWaterMark.value_;
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  DiskUsagePtr metaDiskUsage(
    new DiskUsage(configuration_->metaDataDir.value_,configuration_->metaDataLowWaterMark,configuration_->metaDataHighWaterMark,false)
  );
  resourceManager_->monitorDiskUsage(metaDiskUsage);

  resetMonitoringCounters();
}


void evb::bu::DiskWriter::createDir(const boost::filesystem::path& path)
{
  if ( ! boost::filesystem::exists(path) &&
    ( ! boost::filesystem::create_directories(path) ) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << path.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
  chmod(path.string().c_str(),S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
}


void evb::bu::DiskWriter::removeDir(const boost::filesystem::path& path)
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
    *out << "<td>last lumi section closed</td>"                     << std::endl;
    *out << "<td>" << diskWriterMonitoring_.currentLumiSection << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void evb::bu::DiskWriter::createLockFile(const boost::filesystem::path& runDir) const
{
  const boost::filesystem::path fulockPath( runDir / "fu.lock" );
  const char* path = fulockPath.string().c_str();
  std::ofstream fulock(path);
  fulock << "1 0";
  fulock.close();
  chmod(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

  const boost::filesystem::path monDirPath( runDir / "mon" );
  DiskWriter::createDir(monDirPath);
}


void evb::bu::DiskWriter::writeEoLS
(
  const uint32_t lumiSection,
  const uint32_t fileCount,
  const uint32_t eventCount
) const
{
  std::ostringstream fileNameStream;
  fileNameStream << std::setfill('0') <<
    "EoLS_"  << std::setw(4) << lumiSection <<
    ".jsn";
  const boost::filesystem::path jsonFile = runMetaDataDir_ / fileNameStream.str();

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  const char* path = jsonFile.string().c_str();
  std::ofstream json(path);
  json << "{"                                                         << std::endl;
  json << "   \"data\" : [ \""     << eventCount  << "\", \""
                                   << fileCount   << "\" ],"          << std::endl;
  json << "   \"definition\" : \"" << eolsDefFile_.string()  << "\"," << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""           << std::endl;
  json << "}"                                                         << std::endl;
  json.close();
  chmod(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
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

  const char* path = jsonFile.string().c_str();
  std::ofstream json(path);
  json << "{"                                                                           << std::endl;
  json << "   \"data\" : [ \""     << diskWriterMonitoring_.nbEventsWritten << "\", \""
                                   << diskWriterMonitoring_.nbFiles         << "\", \""
                                   << diskWriterMonitoring_.nbLumiSections  << "\" ],"  << std::endl;
  json << "   \"definition\" : \"" << eorDefFile_.string()  << "\","                    << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""                             << std::endl;
  json << "}"                                                                           << std::endl;
  json.close();
  chmod(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
}


void evb::bu::DiskWriter::defineEoLSjson()
{
  const boost::filesystem::path jsdDir = runMetaDataDir_ / "jsd";
  createDir(jsdDir);
  eolsDefFile_ = jsdDir / "EoLS.jsd";

  const char* path = eolsDefFile_.string().c_str();
  std::ofstream json(path);
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ]"                                              << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
  chmod(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
}


void evb::bu::DiskWriter::defineEoRjson()
{
  const boost::filesystem::path jsdDir = runMetaDataDir_ / "jsd";
  createDir(jsdDir);
  eorDefFile_ = jsdDir / "EoR.jsd";

  const char* path = eorDefFile_.string().c_str();
  std::ofstream json(path);
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
  chmod(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
