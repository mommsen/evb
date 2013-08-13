#include <sstream>
#include <iomanip>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::bu::DiskWriter::DiskWriter(BU* bu) :
bu_(bu),
configuration_(bu->getConfiguration()),
buInstance_( bu->getApplicationDescriptor()->getInstance() ),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
  startLumiMonitoring();
}


evb::bu::StreamHandlerPtr evb::bu::DiskWriter::getStreamHandler(const uint16_t builderId)
{
  return streamHandlers_[builderId];
}


void evb::bu::DiskWriter::startProcessing(const uint32_t runNumber)
{
  if ( configuration_->dropEventData ) return;

  runNumber_ = runNumber;

  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(6) << runNumber_;

  runRawDataDir_ = buRawDataDir_ / runDir.str() / "open";
  DiskWriter::createDir(runRawDataDir_);

  runMetaDataDir_ = buMetaDataDir_ / runDir.str();
  DiskWriter::createDir(runMetaDataDir_);

  for (uint16_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    StreamHandlerPtr streamHandler(
      new StreamHandler(buInstance_,runNumber_,runRawDataDir_,runMetaDataDir_,configuration_)
    );
    streamHandlers_.insert( StreamHandlers::value_type(i,streamHandler) );
  }

  defineEoLSjson();
  defineEoRjson();

  doProcessing_ = true;
  lumiMonitoringWorkLoop_->submit(lumiMonitoringAction_);
}


void evb::bu::DiskWriter::stopProcessing()
{
  if ( configuration_->dropEventData ) return;

  doProcessing_ = false;
  while ( processActive_ ) ::usleep(1000);

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
  catch (xcept::Exception& e)
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

  processActive_ = false;

  ::sleep(5);

  return doProcessing_;
}


void evb::bu::DiskWriter::gatherLumiStatistics()
{
  LumiMonitorPtr lumiMonitor;
  std::pair<LumiMonitors::iterator,bool> result;
  const uint32_t streamCount = streamHandlers_.size();

  for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
       it != itEnd; ++it)
  {
    while ( it->second->getLumiMonitor(lumiMonitor) )
    {
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
        lumiMonitors_.erase(result.first);
      }
    }
  }
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
  clear();

  if ( configuration_->dropEventData ) return;

  std::ostringstream buDir;
  buDir << "BU-" << std::setfill('0') << std::setw(3) << buInstance_;

  buRawDataDir_ = configuration_->rawDataDir.value_;
  buRawDataDir_ /= buDir.str();
  createDir(buRawDataDir_);
  //rawDataDiskUsage_.reset( new DiskUsage(buRawDataDir_, configuration_->rawDataHighWaterMark, configuration_->rawDataLowWaterMark) );

  buMetaDataDir_ = configuration_->metaDataDir.value_;
  buMetaDataDir_ /= buDir.str();
  createDir(buMetaDataDir_);
  //metaDataDiskUsage_.reset( new DiskUsage(buMetaDataDir_, configuration_->metaDataHighWaterMark, configuration_->metaDataLowWaterMark) );
}


void evb::bu::DiskWriter::clear()
{
  streamHandlers_.clear();
  lumiMonitors_.clear();
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


void evb::bu::DiskWriter::printHtml(xgi::Output *out)
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

  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                         << std::endl;
  json << "   \"data\" : [ \""     << eventCount  << "\", \""
                                   << fileCount   << "\" ],"          << std::endl;
  json << "   \"definition\" : \"" << eolsDefFile_.string()  << "\"," << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""           << std::endl;
  json << "}"                                                         << std::endl;
  json.close();
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

  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                                           << std::endl;
  json << "   \"data\" : [ \""     << diskWriterMonitoring_.nbEventsWritten << "\", \""
                                   << diskWriterMonitoring_.nbFiles         << "\", \""
                                   << diskWriterMonitoring_.nbLumiSections  << "\" ],"  << std::endl;
  json << "   \"definition\" : \"" << eorDefFile_.string()  << "\","                    << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""                             << std::endl;
  json << "}"                                                                           << std::endl;
  json.close();
}


void evb::bu::DiskWriter::defineEoLSjson()
{
  eolsDefFile_ = runMetaDataDir_ / "EoLS.jsd";

  std::ofstream json(eolsDefFile_.string().c_str());
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
  json << "   ],"                                             << std::endl;
  json << "   \"file\" : \"" << eolsDefFile_.string() << "\"" << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
}


void evb::bu::DiskWriter::defineEoRjson()
{
  eorDefFile_ = runMetaDataDir_ / "EoR.jsd";

  std::ofstream json(eorDefFile_.string().c_str());
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
  json << "   ],"                                             << std::endl;
  json << "   \"file\" : \"" << eorDefFile_.string() << "\""  << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
