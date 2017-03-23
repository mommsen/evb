#include <boost/filesystem/convenience.hpp>

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "evb/Exception.h"
#include "evb/DataLocations.h"
#include "evb/bu/EventInfo.h"
#include "evb/bu/FedInfo.h"
#include "evb/bu/FileHandler.h"
#include "xcept/tools.h"


evb::bu::FileHandler::FileHandler(const std::string& rawFileName) :
  rawFileName_(rawFileName),
  fileDescriptor_(0),
  fileSize_(0)
{
  if ( boost::filesystem::exists(rawFileName_) )
  {
    std::ostringstream msg;
    msg << "The output file " << rawFileName_ << " already exists";
    XCEPT_RAISE(exception::DiskWriting, msg.str());
  }

  fileDescriptor_ = open(rawFileName_.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  if ( fileDescriptor_ == -1 )
  {
    std::ostringstream msg;
    msg << "Failed to open output file " << rawFileName_
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, msg.str());
  }
}


evb::bu::FileHandler::~FileHandler()
{
  closeAndGetFileSize();
}


void evb::bu::FileHandler::writeEvent(const EventPtr& event)
{
  const EventInfoPtr& eventInfo = event->getEventInfo();
  size_t bytesWritten = write(fileDescriptor_,eventInfo.get(),sizeof(EventInfo));

  const DataLocations& locs = event->getDataLocations();
  bytesWritten += writev(fileDescriptor_,&locs[0],locs.size());

  if ( bytesWritten != sizeof(EventInfo) + eventInfo->eventSize() )
  {
    std::ostringstream msg;
    msg << "Failed to completely write event " << event->getEvBid();
    msg << " into " << rawFileName_;
    XCEPT_RAISE(exception::DiskWriting, msg.str());
  }

  fileSize_ += bytesWritten;
}


uint64_t evb::bu::FileHandler::closeAndGetFileSize()
{
  std::string error = "Failed to close the output file " + rawFileName_;

  try
  {
    if ( fileDescriptor_ )
    {
      if ( ::close(fileDescriptor_) < 0 )
      {
        std::ostringstream msg;
        msg << error << ": " << strerror(errno);
        XCEPT_RAISE(exception::DiskWriting, msg.str());
      }
      fileDescriptor_ = 0;
    }
  }
  catch(std::exception& e)
  {
    error += ": ";
    error += e.what();
    XCEPT_RAISE(exception::DiskWriting, error);
  }
  catch(...)
  {
    error += ": unknown exception";
    XCEPT_RAISE(exception::DiskWriting, error);
  }

  return fileSize_;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
