#include <iostream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "evb/bu/DiskUsage.h"
#include "evb/Exception.h"
#include "xcept/tools.h"


evb::bu::DiskUsage::DiskUsage
(
  const boost::filesystem::path& path,
  const float lowWaterMark,
  const float highWaterMark,
  const bool deleteFiles
) :
  path_(path),
  lowWaterMark_(lowWaterMark),
  highWaterMark_(highWaterMark),
  deleteFiles_(deleteFiles),
  state_(IDLE),
  valid_(false)
{
  if ( lowWaterMark >= highWaterMark )
  {
    std::ostringstream msg;
    msg << "The lowWaterMark " << lowWaterMark;
    msg << " must be smaller than the highWaterMark " << highWaterMark;
    msg << " for " << path;
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  diskUsageThread_ = boost::thread( boost::bind( &DiskUsage::doStatFs, this ) );
  deleteThread_ = boost::thread( boost::bind( &DiskUsage::deleteRawFiles, this ) );
  update();
}


evb::bu::DiskUsage::~DiskUsage()
{
  deleteFiles_ = false;
  {
    boost::mutex::scoped_lock lock(mutex_);
    state_ = STOP;
    condition_.notify_one();
  }
  try
  {
    diskUsageThread_.join();
    deleteThread_.join();
  }
  catch ( const boost::thread_interrupted& ) {}
}


void evb::bu::DiskUsage::update()
{
  boost::mutex::scoped_lock lock(mutex_);

  if ( state_ != IDLE )
  {
    valid_ = false;
    return;
  }

  state_ = UPDATE;
  condition_.notify_one();
}


float evb::bu::DiskUsage::overThreshold()
{
  const float diskUsage = relDiskUsage();

  if ( ! valid_ ) return 1; //error condition

  return (diskUsage < lowWaterMark_) ? 0 :
    (diskUsage - lowWaterMark_) / (highWaterMark_ - lowWaterMark_);
}


float evb::bu::DiskUsage::diskSizeGB()
{
  return ( valid_ ? diskSizeGB_ : -1 );
}


float evb::bu::DiskUsage::relDiskUsage()
{
  return ( valid_ ? relDiskUsage_ : -1 );
}


void evb::bu::DiskUsage::doStatFs()
{
  while(1)
  {
    {
      boost::mutex::scoped_lock lock(mutex_);
      while (state_ == IDLE)
      {
        condition_.wait(lock);
      }
      if (state_ == STOP) break;
      state_ = UPDATING;
    }

    struct statfs64 statfs;
    const int retVal = statfs64(path_.string().c_str(), &statfs);

    if ( retVal == 0 )
    {
      diskSizeGB_ = static_cast<float>(statfs.f_blocks * statfs.f_bsize) / 1000 / 1000 / 1000;
      if ( statfs.f_blocks > statfs.f_bfree )
        relDiskUsage_ = 1 - static_cast<float>(statfs.f_bfree)/statfs.f_blocks;
      else
        relDiskUsage_ = 1;
      valid_ = true;
    }

    if (path_ == "/aSlowDiskForUnitTests") ::sleep(5);

    {
      boost::mutex::scoped_lock lock(mutex_);
      if (state_ == STOP) break;
      state_ = IDLE;
    }
  }
}


void evb::bu::DiskUsage::deleteRawFiles()
{
  while ( deleteFiles_ )
  {
    if ( valid_ && relDiskUsage_ > 0.5 )
    {
      boost::filesystem::recursive_directory_iterator it(path_);
      while ( it != boost::filesystem::recursive_directory_iterator() )
      {
        if ( it->path().filename() == "open" )
        {
          it.pop();
          continue;
        }

        if ( it->path().extension() == ".raw" )
        {
          boost::filesystem::remove(*it);
        }
        ++it;
      }
    }
    else
    {
      ::sleep(1);
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
