#include <sstream>
#include <iomanip>

#include "evb/BU.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/LumiHandler.h"
#include "evb/bu/StateMachine.h"
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
buInstance_( bu_->getApplicationDescriptor()->getInstance() ),
eventFIFO_("eventFIFO"),
eolsFIFO_("eolsFIFO"),
fileHandlerAndEventFIFO_("fileHandlerAndEventFIFO"),
writingActive_(false),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


evb::bu::DiskWriter::~DiskWriter()
{
}


void evb::bu::DiskWriter::writeEvent(const EventPtr event)
{
  if ( configuration_->dropEventData )
  {
    resourceManager_->discardEvent(event);
  }
  else
  {
    while ( ! eventFIFO_.enq(event) ) { ::usleep(1000); }
  }
}


void evb::bu::DiskWriter::closeLS(const uint32_t lumiSection)
{
  if ( configuration_->dropEventData ) return;

  while ( ! eolsFIFO_.enq(lumiSection) ) { ::usleep(1000); }
}


void evb::bu::DiskWriter::startProcessingWorkLoop()
{
  try
  {
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("DiskWriterProcessing"), "waiting" );

    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &evb::bu::DiskWriter::process,
          bu_->getIdentifier("diskWriterProcess") );

      processingWL_->activate();
    }

    resourceMonitoringWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("DiskWriterResourceMonitoring"), "waiting" );

    if ( ! resourceMonitoringWL_->isActive() )
    {
      resourceMonitoringAction_ =
        toolbox::task::bind(this, &evb::bu::DiskWriter::resourceMonitoring,
          bu_->getIdentifier("resourceMonitoringProcess") );

      resourceMonitoringWL_->activate();
    }

    writingAction_ =
      toolbox::task::bind(this, &evb::bu::DiskWriter::writing,
        bu_->getIdentifier("diskWriting") );
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void evb::bu::DiskWriter::startProcessing(const uint32_t runNumber)
{
  if ( configuration_->dropEventData ) return;

  runNumber_ = runNumber;

  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(8) << runNumber_;

  runRawDataDir_ = buRawDataDir_ / runDir.str() / "open";
  if ( ! boost::filesystem::create_directories(runRawDataDir_) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << runRawDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  runMetaDataDir_ = buMetaDataDir_ / runDir.str();
  if ( ! boost::filesystem::exists(runMetaDataDir_) &&
    ( ! boost::filesystem::create_directories(runMetaDataDir_) ) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << runMetaDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  doProcessing_ = true;
  processingWL_->submit(processingAction_);
  resourceMonitoringWL_->submit(resourceMonitoringAction_);

  for (uint32_t i=0; i < configuration_->numberOfWriters; ++i)
  {
    writingWorkLoops_.at(i)->submit(writingAction_);
  }
}


void evb::bu::DiskWriter::stopProcessing()
{
  if ( configuration_->dropEventData ) return;

  doProcessing_ = false;

  while ( processActive_ || writingActive_ ) ::usleep(1000);

  for (LumiHandlers::const_iterator it = lumiHandlers_.begin(), itEnd = lumiHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->close();
  }
  lumiHandlers_.clear();

  if ( boost::filesystem::exists(runRawDataDir_) &&
    ( ! boost::filesystem::remove(runRawDataDir_) ) )
  {
    std::ostringstream oss;
    oss << "Failed to remove directory " << runRawDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  writeJSON();
}


bool evb::bu::DiskWriter::process(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  processActive_ = true;

  try
  {
    while (
      doProcessing_ && (
        handleEvents() ||
        handleEoLS()
      )
    ) {};
  }
  catch(xcept::Exception& e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  processActive_ = false;

  return doProcessing_;
}


bool evb::bu::DiskWriter::handleEvents()
{
  EventPtr event;
  if ( ! eventFIFO_.deq(event) ) return false;

  if ( event->runNumber() != runNumber_ )
  {
    std::ostringstream oss;
    oss << "Received an event with run number " << event->runNumber();
    oss << " while expecting events from run " << runNumber_;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  const LumiHandlerPtr lumiHandler = getLumiHandler( event->lumiSection() );
  const FileHandlerPtr fileHandler = lumiHandler->getFileHandler(stateMachine_);

  if ( fileHandler->getAllocatedEventCount() == 1 )
    ++diskWriterMonitoring_.nbFiles;

  FileHandlerAndEventPtr fileHandlerAndEvent(
    new FileHandlerAndEvent(fileHandler, event)
  );
  while ( ! fileHandlerAndEventFIFO_.enq(fileHandlerAndEvent) ) { ::usleep(1000); }

  return true;
}


evb::bu::LumiHandlerPtr evb::bu::DiskWriter::getLumiHandler(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock handlerSL(lumiHandlersMutex_);

  LumiHandlers::iterator pos = lumiHandlers_.lower_bound(lumiSection);

  if ( pos == lumiHandlers_.end() || (lumiHandlers_.key_comp()(lumiSection,pos->first)) )
  {
    // New lumi section
    const LumiHandlerPtr lumiHandler(new LumiHandler(
        buInstance_, runRawDataDir_, runMetaDataDir_, lumiSection, configuration_->maxEventsPerFile, configuration_->numberOfWriters));
    pos = lumiHandlers_.insert(pos, LumiHandlers::value_type(lumiSection, lumiHandler));

    boost::mutex::scoped_lock monitorSL(diskWriterMonitoringMutex_);
    ++diskWriterMonitoring_.nbLumiSections;
    diskWriterMonitoring_.currentLumiSection = lumiSection;
  }

  return pos->second;
}


bool evb::bu::DiskWriter::handleEoLS()
{
  uint32_t lumiSection;
  if ( ! eolsFIFO_.deq(lumiSection) ) return false;

  boost::mutex::scoped_lock handlerSL(lumiHandlersMutex_);
  boost::mutex::scoped_lock monitorSL(diskWriterMonitoringMutex_);

  LumiHandlers::iterator pos = lumiHandlers_.find(lumiSection);

  if ( lumiSection > diskWriterMonitoring_.lastEoLS )
    diskWriterMonitoring_.lastEoLS = lumiSection;

  if ( pos == lumiHandlers_.end() )
  {
    // No events have been written for this lumi section
    // Use a dummy FileHandler to create an empty file
    LumiHandler emptyLumi(buInstance_, runRawDataDir_, runMetaDataDir_, lumiSection, 0, 0);
    emptyLumi.close();
    ++diskWriterMonitoring_.nbLumiSections;
  }
  else
  {
    pos->second->close();
    lumiHandlers_.erase(pos);
  }

  return true;
}


bool evb::bu::DiskWriter::writing(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  writingActive_ = true;

  try
  {
    FileHandlerAndEventPtr fileHandlerAndEvent;
    bool gotEvent(false);
    do
    {
      {
        boost::mutex::scoped_lock sl(fileHandlerAndEventFIFOmutex_);
        gotEvent = fileHandlerAndEventFIFO_.deq(fileHandlerAndEvent);
      }
      if (gotEvent)
      {
        try
        {
          fileHandlerAndEvent->event->checkEvent();
        }
        catch(exception::SuperFragment &e)
        {
          boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
          ++diskWriterMonitoring_.nbEventsCorrupted;
          if ( configuration_->tolerateCorruptedEvents )
          {
            LOG4CPLUS_ERROR(bu_->getApplicationLogger(),
              xcept::stdformat_exception_history(e));
            bu_->notifyQualified("error",e);
          }
          else
          {
            throw(e);
          }
        }

        fileHandlerAndEvent->event->writeToDisk(fileHandlerAndEvent->fileHandler);
        resourceManager_->discardEvent(fileHandlerAndEvent->event);

        boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
        ++diskWriterMonitoring_.nbEventsWritten;
        const uint32_t eventNumber = fileHandlerAndEvent->event->getEvBid().eventNumber();
        if ( eventNumber > diskWriterMonitoring_.lastEventNumberWritten )
          diskWriterMonitoring_.lastEventNumberWritten = eventNumber;
      }
    }
    while (gotEvent);
  }
  catch(xcept::Exception& e)
  {
    writingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  writingActive_ = false;

  return doProcessing_;
}


bool evb::bu::DiskWriter::resourceMonitoring(toolbox::task::WorkLoop*)
{
  bool allOkay(true);

  allOkay &= checkDiskSize(rawDataDiskUsage_);
  allOkay &= checkDiskSize(metaDataDiskUsage_);

  //eventBuilder_->requestEvents(allOkay);

  ::sleep(5);

  return doProcessing_;
}


bool evb::bu::DiskWriter::checkDiskSize(DiskUsagePtr diskUsage)
{
  return ( diskUsage->update() && ! diskUsage->tooHigh() );
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
  nbEvtsCorrupted_ = 0;

  items.add("nbEvtsWritten", &nbEvtsWritten_);
  items.add("nbFilesWritten", &nbFilesWritten_);
  items.add("nbEvtsCorrupted", &nbEvtsCorrupted_);
}


void evb::bu::DiskWriter::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);

  nbEvtsWritten_ = diskWriterMonitoring_.nbEventsWritten;
  nbFilesWritten_ = diskWriterMonitoring_.nbFiles;
  nbEvtsCorrupted_ = diskWriterMonitoring_.nbEventsCorrupted;
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
  diskWriterMonitoring_.nbEventsCorrupted = 0;
}


void evb::bu::DiskWriter::configure()
{
  clear();

  eventFIFO_.resize(configuration_->maxEvtsUnderConstruction);
  eolsFIFO_.resize(configuration_->eolsFIFOCapacity);
  fileHandlerAndEventFIFO_.resize(configuration_->maxEvtsUnderConstruction);

  if ( ! configuration_->dropEventData )
  {
    std::ostringstream buDir;
    buDir << "BU-" << std::setfill('0') << std::setw(3) << buInstance_;

    buRawDataDir_ = configuration_->rawDataDir.value_;
    buRawDataDir_ /= buDir.str();
    if ( ! boost::filesystem::exists(buRawDataDir_) &&
      ( ! boost::filesystem::create_directories(buRawDataDir_) ) )
    {
      std::ostringstream oss;
      oss << "Failed to create directory " << buRawDataDir_.string();
      XCEPT_RAISE(exception::DiskWriting, oss.str());
    }
    rawDataDiskUsage_.reset( new DiskUsage(buRawDataDir_, configuration_->rawDataHighWaterMark, configuration_->rawDataLowWaterMark) );

    buMetaDataDir_ = configuration_->metaDataDir.value_;
    buMetaDataDir_ /= buDir.str();
    if ( ! boost::filesystem::exists(buMetaDataDir_) &&
      ( ! boost::filesystem::create_directories(buMetaDataDir_) ) )
    {
      std::ostringstream oss;
      oss << "Failed to create directory " << buMetaDataDir_.string();
      XCEPT_RAISE(exception::DiskWriting, oss.str());
    }
    metaDataDiskUsage_.reset( new DiskUsage(buMetaDataDir_, configuration_->metaDataHighWaterMark, configuration_->metaDataLowWaterMark) );

    createWritingWorkLoops();
  }
}


void evb::bu::DiskWriter::createWritingWorkLoops()
{
  const std::string identifier = bu_->getIdentifier();

  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=writingWorkLoops_.size(); i < configuration_->numberOfWriters; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "DiskWriter_" << i;
      toolbox::task::WorkLoop* wl = toolbox::task::getWorkLoopFactory()->getWorkLoop( workLoopName.str(), "waiting" );

      if ( ! wl->isActive() ) wl->activate();
      writingWorkLoops_.push_back(wl);
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void evb::bu::DiskWriter::clear()
{
  eventFIFO_.clear();
  eolsFIFO_.clear();
  fileHandlerAndEventFIFO_.clear();
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
    *out << "<td># corrupted events</td>"                           << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbEventsCorrupted << "</td>" << std::endl;
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
  if ( rawDataDiskUsage_.get() )
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Raw-data disk size (GB)</td>"                      << std::endl;
    *out << "<td>" << rawDataDiskUsage_->diskSizeGB() << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Raw-data disk usage</td>"                          << std::endl;
    *out << "<td>" << rawDataDiskUsage_->relDiskUsage() << "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  if ( metaDataDiskUsage_.get() )
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Meta-data disk size (GB)</td>"                     << std::endl;
    *out << "<td>" << metaDataDiskUsage_->diskSizeGB() << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Meta-data disk usage</td>"                         << std::endl;
    *out << "<td>" << metaDataDiskUsage_->relDiskUsage() << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  eventFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  fileHandlerAndEventFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  eolsFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
