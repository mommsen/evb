#include <sstream>

#include "evb/bu/StreamHandler.h"
#include "evb/Exception.h"


evb::bu::StreamHandler::StreamHandler
(
  const std::string& streamFileName,
  ConfigurationPtr configuration
) :
streamFileName_(streamFileName),
configuration_(configuration),
index_(0),
currentFileStatistics_(new FileStatistics(0,"")),
fileStatisticsFIFO_("currentFileStatisticsFIFO")
{
  fileStatisticsFIFO_.resize(configuration->fileStatisticsFIFOCapacity);
}


evb::bu::StreamHandler::~StreamHandler()
{
  closeFile();
}


void evb::bu::StreamHandler::writeEvent(const EventPtr event)
{
  boost::mutex::scoped_lock sl(fileHandlerMutex_);

  const uint32_t lumiSection = event->lumiSection();

  if ( fileHandler_.get() && lumiSection > currentFileStatistics_->lumiSection )
  {
    do_closeFile();
  }
  else if ( lumiSection < currentFileStatistics_->lumiSection )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << currentFileStatistics_->lumiSection;
    oss << " with evb id " << event->getEvBid();
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( ! fileHandler_.get() )
  {
    std::ostringstream fileName;
    fileName << streamFileName_ << "_" << std::hex << static_cast<unsigned int>(++index_);
    fileHandler_.reset( new FileHandler(fileName.str()) );
    currentFileStatistics_.reset( new FileStatistics(lumiSection,fileName.str()) );
  }

  event->writeToDisk(fileHandler_, configuration_->calculateAdler32);
  currentFileStatistics_->lastEventNumberWritten = event->eventNumber();

  if ( ++currentFileStatistics_->nbEventsWritten >= configuration_->maxEventsPerFile )
  {
    do_closeFile();
  }

  ::usleep(configuration_->sleepBetweenEvents);
}


bool evb::bu::StreamHandler::closeFileIfOpenedBefore(const time_t& time)
{
  boost::mutex::scoped_lock sl(fileHandlerMutex_);

  if ( fileHandler_.get() && currentFileStatistics_->creationTime < time )
  {
    do_closeFile();
    return true;
  }

  return false;
}


void evb::bu::StreamHandler::closeFile()
{
  boost::mutex::scoped_lock sl(fileHandlerMutex_);

  if ( fileHandler_.get() )
  {
    do_closeFile();
  }
}


void evb::bu::StreamHandler::do_closeFile()
{
  currentFileStatistics_->fileSize =
    fileHandler_->closeAndGetFileSize();

  fileStatisticsFIFO_.enqWait(currentFileStatistics_);

  fileHandler_.reset();
}


bool evb::bu::StreamHandler::getFileStatistics(FileStatisticsPtr& fileStatistics)
{
  return fileStatisticsFIFO_.deq(fileStatistics);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
