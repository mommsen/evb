#include <sstream>
#include <iomanip>

#include "evb/bu/DiskWriter.h"
#include "evb/bu/StreamHandler.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/LumiHandler.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::bu::StreamHandler::StreamHandler
(
  const uint32_t buInstance,
  const uint32_t builderId,
  const uint32_t runNumber,
  const boost::filesystem::path& runRawDataDir,
  const boost::filesystem::path& runMetaDataDir,
  ConfigurationPtr configuration
) :
buInstance_(buInstance),
builderId_(builderId),
runNumber_(runNumber),
runRawDataDir_(runRawDataDir),
runMetaDataDir_(runMetaDataDir),
maxEventsPerFile_(configuration->maxEventsPerFile),
numberOfBuilders_(configuration->numberOfBuilders),
currentLumiSection_(0),
index_(builderId)
{
}


evb::bu::StreamHandler::~StreamHandler()
{
  close();
}


void evb::bu::StreamHandler::close()
{
  fileHandler_.reset();
}


void evb::bu::StreamHandler::writeEvent(const EventPtr event)
{
  const uint32_t lumiSection = event->lumiSection();
  if ( lumiSection > currentLumiSection_ )
  {
    closeLumiSection();
    currentLumiSection_ = lumiSection;
    index_ = builderId_;
  }
  else if ( lumiSection < currentLumiSection_ )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << currentLumiSection_;
    oss << " with evb id " << event->getEvBid();
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( fileHandler_.get() == 0 || fileHandler_->getEventCount() >= maxEventsPerFile_ )
  {
    fileHandler_.reset( new FileHandler(buInstance_, runRawDataDir_, runMetaDataDir_, currentLumiSection_, index_) );
    index_ += numberOfBuilders_;
  }

  event->writeToDisk(fileHandler_);
}


void evb::bu::StreamHandler::closeLumiSection()
{
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
