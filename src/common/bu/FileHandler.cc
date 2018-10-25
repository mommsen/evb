#include <boost/filesystem/convenience.hpp>

#include <iomanip>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "evb/Exception.h"
#include "evb/DataLocations.h"
#include "evb/bu/EventInfo.h"
#include "evb/bu/FedInfo.h"
#include "evb/bu/FileHandler.h"
#include "xcept/tools.h"


evb::bu::FileHandler::FileHandler(const std::string& rawFileName) :
  rawFileName_(rawFileName),
  fileDescriptor_(0),
  fileSize_(0),
  maxFileSize_(100*1*1000000)
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

  // Stretch file size to hold all events
  // TODO: add configuration parameters
  // 100 events of 3 MB
  const int result = lseek(fileDescriptor_, maxFileSize_, SEEK_SET);
  if ( result == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to stretch " << rawFileName_
      << " by " << maxFileSize_ << " Bytes: " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  // Something needs to be written at the end of the file to
  // have the file actually have the new size.
  // Just writing an empty string at the current file position will do.
  if ( ::write(fileDescriptor_, "", 1) != 1 )
  {
    std::ostringstream oss;
    oss << "Failed to write last byte to " << rawFileName_
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileMap_ = static_cast<unsigned char*>( mmap(0, maxFileSize_, PROT_WRITE, MAP_SHARED|MAP_NORESERVE, fileDescriptor_, 0) );
  if (fileMap_ == MAP_FAILED)
  {
    std::ostringstream oss;
    oss << "Failed to mmap " << rawFileName_
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
  memcpy(fileMap_+fileSize_, eventInfo.get(), sizeof(EventInfo));
  fileSize_ += sizeof(EventInfo);

  const DataLocations& locs = event->getDataLocations();
  for (DataLocations::const_iterator it = locs.begin(); it != locs.end(); ++it)
  {
    memcpy(fileMap_+fileSize_, it->iov_base, it->iov_len);
    fileSize_ += it->iov_len;
  }
}


uint64_t evb::bu::FileHandler::closeAndGetFileSize()
{
  std::string error = "Failed to close the output file " + rawFileName_;

  try
  {
    if ( fileDescriptor_ )
    {
      if ( munmap(fileMap_, maxFileSize_) == -1 )
      {
        std::ostringstream msg;
        msg << "Failed to unmap " << rawFileName_
          << ": " << strerror(errno);
        XCEPT_RAISE(exception::DiskWriting, msg.str());
      }
      fileMap_ = 0;

      if ( ftruncate(fileDescriptor_, fileSize_) == -1 )
      {
        std::ostringstream msg;
        msg << "Failed to truncate " << rawFileName_
          << " to " << fileSize_ << " Bytes"
          << ": " << strerror(errno);
        XCEPT_RAISE(exception::DiskWriting, msg.str());
      }

      if ( ::close(fileDescriptor_) < 0 )
      {
        std::ostringstream msg;
        msg << "Failed to close " << rawFileName_
          << ": " << strerror(errno);
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
