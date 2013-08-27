#include <sstream>
#include <iomanip>

#include "evb/bu/DiskWriter.h"
#include "evb/bu/StreamHandler.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::bu::StreamHandler::StreamHandler
(
  const uint32_t buInstance,
  const uint32_t runNumber,
  const boost::filesystem::path& runRawDataDir,
  const boost::filesystem::path& runMetaDataDir,
  ConfigurationPtr configuration
) :
buInstance_(buInstance),
runNumber_(runNumber),
runRawDataDir_(runRawDataDir),
runMetaDataDir_(runMetaDataDir),
configuration_(configuration),
currentLumiMonitor_( new LumiMonitor(1) ),
lumiMonitorFIFO_("lumiMonitorFIFO")
{
  lumiMonitorFIFO_.resize(configuration->lumiMonitorFIFOCapacity);
}


evb::bu::StreamHandler::~StreamHandler()
{
  close();
}


void evb::bu::StreamHandler::close()
{
  fileHandler_.reset();

  boost::mutex::scoped_lock sl(currentLumiMonitorMutex_);
  if ( currentLumiMonitor_.get() )
  {
    lumiMonitorFIFO_.enqWait(currentLumiMonitor_);
    currentLumiMonitor_.reset();
  }
}


void evb::bu::StreamHandler::writeEvent(const EventPtr event)
{
  boost::mutex::scoped_lock sl(currentLumiMonitorMutex_);

  const uint32_t lumiSection = event->lumiSection();

  if ( lumiSection > currentLumiMonitor_->lumiSection )
  {
    closeLumiSection(lumiSection);
  }
  else if ( lumiSection < currentLumiMonitor_->lumiSection )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << currentLumiMonitor_->lumiSection;
    oss << " with evb id " << event->getEvBid();
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( fileHandler_.get() == 0 )
  {
    fileHandler_.reset( new FileHandler(buInstance_, runNumber_, runRawDataDir_, runMetaDataDir_, lumiSection) );
    ++(currentLumiMonitor_->nbFiles);
  }

  event->writeToDisk(fileHandler_);

  currentLumiMonitor_->update( event->getEvBid().eventNumber() );

  if ( fileHandler_->getEventCount() >= configuration_->maxEventsPerFile )
  {
    fileHandler_.reset();
  }
}


void evb::bu::StreamHandler::closeLumiSection(const uint32_t lumiSection)
{
  fileHandler_.reset();

  lumiMonitorFIFO_.enqWait(currentLumiMonitor_);

  // Create empty lumiMonitors for skipped lumi sections
  for (uint32_t ls = currentLumiMonitor_->lumiSection+1; ls < lumiSection; ++ls)
    lumiMonitorFIFO_.enqWait( LumiMonitorPtr(new LumiMonitor(ls)) );

  currentLumiMonitor_.reset( new LumiMonitor(lumiSection) );
}


bool evb::bu::StreamHandler::getLumiMonitor(LumiMonitorPtr& lumiMonitor)
{
  if ( lumiMonitorFIFO_.deq(lumiMonitor) ) return true;

  // Check for old lumi sections
  boost::mutex::scoped_lock sl(currentLumiMonitorMutex_);

  if ( ! currentLumiMonitor_.get() ) return false;

  if ( time(0) > (currentLumiMonitor_->timeOfLastEvent + configuration_->lumiSectionTimeout) )
  {
    fileHandler_.reset();
    lumiMonitor = currentLumiMonitor_;
    currentLumiMonitor_.reset( new LumiMonitor(lumiMonitor->lumiSection+1) );
    return true;
  }

  return false;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
