#include <sstream>

#include "evb/BU.h"
#include "evb/bu/StreamHandler.h"
#include "evb/Exception.h"


evb::bu::StreamHandler::StreamHandler
(
  BU* bu,
  const std::string& streamFileName
) :
  streamFileName_(streamFileName),
  configuration_(bu->getConfiguration()),
  index_(0),
  currentFileStatistics_(new FileStatistics(0,"")),
  fileStatisticsFIFO_(bu,"fileStatisticsFIFO_"+streamFileName.substr(streamFileName.rfind("/")+1))
{
  fileStatisticsFIFO_.resize(configuration_->fileStatisticsFIFOCapacity);
}


evb::bu::StreamHandler::~StreamHandler()
{
  closeFile();
}


void evb::bu::StreamHandler::writeEvent(const EventPtr event)
{
  if ( configuration_->dropEventData ) return;

  std::lock_guard<std::mutex> guard(fileHandlerMutex_);

  const uint32_t lumiSection = event->getEventInfo()->lumiSection();

  if ( fileHandler_.get() && lumiSection > currentFileStatistics_->lumiSection )
  {
    do_closeFile();
  }
  else if ( lumiSection < currentFileStatistics_->lumiSection )
  {
    std::ostringstream msg;
    msg << "Received an event from an earlier lumi section " << lumiSection;
    msg << " while processing lumi section " << currentFileStatistics_->lumiSection;
    msg << " with evb id " << event->getEvBid();
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  if ( ! fileHandler_.get() )
  {
    std::ostringstream fileName;
    fileName << streamFileName_ << "_" << std::hex << static_cast<unsigned int>(++index_);
    fileHandler_.reset( new FileHandler(fileName.str()) );
    currentFileStatistics_.reset( new FileStatistics(lumiSection,fileName.str()) );
  }

  fileHandler_->writeEvent(event);
  currentFileStatistics_->lastEventNumberWritten = event->getEventInfo()->eventNumber();

  if ( ++currentFileStatistics_->nbEventsWritten >= configuration_->maxEventsPerFile )
  {
    do_closeFile();
  }
}


bool evb::bu::StreamHandler::closeFileIfOpenedBefore(const time_t& time)
{
  std::lock_guard<std::mutex> guard(fileHandlerMutex_);

  if ( fileHandler_.get() && currentFileStatistics_->creationTime < time )
  {
    do_closeFile();
    return true;
  }

  return false;
}


void evb::bu::StreamHandler::closeFile()
{
  std::lock_guard<std::mutex> guard(fileHandlerMutex_);

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
