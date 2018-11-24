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
  fileStatisticsFIFO_(bu,"fileStatisticsFIFO_"+streamFileName.substr(streamFileName.rfind("/")+1)),
  streamBufferSizeGB_(45)
{
  fileStatisticsFIFO_.resize(configuration_->fileStatisticsFIFOCapacity);
  streamBuffer_ = static_cast<unsigned char*>( malloc(streamBufferSizeGB_*1000*1000*1000) );
  //bzero(streamBuffer_,streamBufferSizeGB_*1000*1000*1000);
  writePtr_ = readPtr_ = streamBuffer_;
}


evb::bu::StreamHandler::~StreamHandler()
{
  closeFile();
}


void evb::bu::StreamHandler::writeEvent(const EventPtr event)
{
  if ( configuration_->dropEventData ) return;

  boost::mutex::scoped_lock sl(fileHandlerMutex_);

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

  //fileHandler_->writeEvent(event);

  const EventInfoPtr& eventInfo = event->getEventInfo();
  copyDataIntoBuffer(eventInfo.get(), sizeof(EventInfo));

  const DataLocations& locs = event->getDataLocations();
  for (DataLocations::const_iterator it = locs.begin(); it != locs.end(); ++it)
  {
    copyDataIntoBuffer(it->iov_base,it->iov_len);
  }
  currentFileStatistics_->lastEventNumberWritten = event->getEventInfo()->eventNumber();

  if ( ++currentFileStatistics_->nbEventsWritten >= configuration_->maxEventsPerFile )
  {
    do_closeFile();
  }
}


void evb::bu::StreamHandler::copyDataIntoBuffer(void* loc, size_t length)
{
  const size_t remaining = streamBuffer_ + streamBufferSizeGB_*1000*1000*1000 - writePtr_;

  if ( length > remaining )
  {
    memcpy(writePtr_,loc,remaining);
    memcpy(streamBuffer_,static_cast<unsigned char*>(loc)+remaining,length-remaining);
    writePtr_ = streamBuffer_ + length - remaining;
  }
  else
  {
    memcpy(writePtr_,loc,length);
    writePtr_ += length;
  }
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
