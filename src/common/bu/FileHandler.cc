#include <boost/filesystem/convenience.hpp>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "evb/Exception.h"
#include "evb/bu/FileHandler.h"
#include "xcept/tools.h"


evb::bu::FileHandler::FileHandler(FileStatisticsPtr fileStatistics) :
fileStatistics_(fileStatistics),
fileDescriptor_(0)
{
  if ( boost::filesystem::exists(fileStatistics_->fileName) )
  {
    std::ostringstream oss;
    oss << "The output file " << fileStatistics_->fileName << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileDescriptor_ = open(fileStatistics_->fileName.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  if ( fileDescriptor_ == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to open output file " << fileStatistics_->fileName
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}

void evb::bu::FileHandler::eventWritten(const uint32_t eventNumber)
{
  if ( fileStatistics_->lastEventNumberWritten < eventNumber )
    fileStatistics_->lastEventNumberWritten = eventNumber;
  ++fileStatistics_->nbEventsWritten;
}


void* evb::bu::FileHandler::getMemMap(const size_t length)
{
  const int result = lseek(fileDescriptor_, fileStatistics_->fileSize+length-1, SEEK_SET);
  if ( result == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to stretch the output file " << fileStatistics_->fileName
      << " by " << length << " Bytes: " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  // Something needs to be written at the end of the file to
  // have the file actually have the new size.
  // Just writing an empty string at the current file position will do.
  if ( ::write(fileDescriptor_, "", 1) != 1 )
  {
    std::ostringstream oss;
    oss << "Failed to write last byte to output file " << fileStatistics_->fileName
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  void* map = mmap(0, length, PROT_WRITE, MAP_SHARED, fileDescriptor_, fileStatistics_->fileSize);
  if (map == MAP_FAILED)
  {
    std::ostringstream oss;
    oss << "Failed to mmap the output file " << fileStatistics_->fileName
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileStatistics_->fileSize += length;

  return map;
}


evb::bu::FileStatisticsPtr evb::bu::FileHandler::closeAndGetFileStatistics()
{
  std::string msg = "Failed to close the output file " + fileStatistics_->fileName;

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

  return fileStatistics_;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
