#include <boost/filesystem/convenience.hpp>

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "evb/Exception.h"
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
    std::ostringstream oss;
    oss << "The output file " << rawFileName_ << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileDescriptor_ = open(rawFileName_.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  if ( fileDescriptor_ == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to open output file " << rawFileName_
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}


evb::bu::FileHandler::~FileHandler()
{
  closeAndGetFileSize();
}


void evb::bu::FileHandler::writeEvent(const EventPtr& event)
{
  const EventInfoPtr& eventInfo = event->getEventInfo();
  write(fileDescriptor_,eventInfo.get(),sizeof(EventInfo));

  const FedInfo::DataLocations& locs = event->getDataLocations();
  writev(fileDescriptor_,&locs[0],locs.size());

  fileSize_ += sizeof(EventInfo) + eventInfo->eventSize();
}


uint64_t evb::bu::FileHandler::closeAndGetFileSize()
{
  std::string msg = "Failed to close the output file " + rawFileName_;

  try
  {
    if ( fileDescriptor_ )
    {
      if ( ::close(fileDescriptor_) < 0 )
      {
        std::ostringstream oss;
        oss << msg << ": " << strerror(errno);
        XCEPT_RAISE(exception::DiskWriting, oss.str());
      }
      fileDescriptor_ = 0;
    }
  }
  catch(std::exception& e)
  {
    msg += ": ";
    msg += e.what();
    XCEPT_RAISE(exception::DiskWriting, msg);
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_RAISE(exception::DiskWriting, msg);
  }

  return fileSize_;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
