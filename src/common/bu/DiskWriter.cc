#include <sstream>
#include <iomanip>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/Exception.h"


evb::bu::DiskWriter::DiskWriter(BU* bu) :
configuration_(bu->getConfiguration()),
buInstance_( bu->getApplicationDescriptor()->getInstance() )
{
  resetMonitoringCounters();
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
      new StreamHandler(buInstance_,i,runNumber_,runRawDataDir_,runMetaDataDir_,configuration_)
    );
    streamHandlers_.insert( StreamHandlers::value_type(i,streamHandler) );
  }
}


void evb::bu::DiskWriter::stopProcessing()
{
  if ( configuration_->dropEventData ) return;

  for (StreamHandlers::const_iterator it = streamHandlers_.begin(), itEnd = streamHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->close();
  }
  streamHandlers_.clear();

  writeJSON();

  removeDir(runRawDataDir_);
}


void evb::bu::DiskWriter::writeJSON()
{
  const boost::filesystem::path jsonDefFile = runMetaDataDir_ / "EoR.jsd";
  defineJSON(jsonDefFile);

  std::ostringstream fileNameStream;
  fileNameStream
    << "EoR_" << std::setfill('0') << std::setw(8) << runNumber_ << ".jsn";
  const boost::filesystem::path jsonFile = runMetaDataDir_ / fileNameStream.str();

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists.";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                                           << std::endl;
  json << "   \"Data\" : [ \""     << diskWriterMonitoring_.nbEventsWritten << "\", \""
                                   << diskWriterMonitoring_.nbFiles         << "\", \""
                                   << diskWriterMonitoring_.nbLumiSections  << "\" ],"  << std::endl;
  json << "   \"Definition\" : \"" << jsonDefFile.string()  << "\","                    << std::endl;
  json << "   \"Source\" : \"BU-"  << buInstance_   << "\""                             << std::endl;
  json << "}"                                                                           << std::endl;
  json.close();
}


void evb::bu::DiskWriter::defineJSON(const boost::filesystem::path& jsonDefFile) const
{
  std::ofstream json(jsonDefFile.string().c_str());
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
  json << "   \"file\" : \"" << jsonDefFile.string() << "\""  << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
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
  diskWriterMonitoring_.lastEoLS = 0;
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
{}


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
    *out << "<td># lumi sections</td>"                              << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbLumiSections << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>current lumi section</td>"                         << std::endl;
    *out << "<td>" << diskWriterMonitoring_.currentLumiSection << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last EoLS signal</td>"                             << std::endl;
    *out << "<td>" << diskWriterMonitoring_.lastEoLS << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
