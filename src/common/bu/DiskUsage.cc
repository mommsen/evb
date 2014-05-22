#include <iostream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

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
  valid_(false)
{
  if ( lowWaterMark >= highWaterMark )
  {
    std::ostringstream oss;
    oss << "The lowWaterMark " << lowWaterMark;
    oss << " must be smaller than the highWaterMark " << highWaterMark;
    oss << " for " << path;
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  update();
}


evb::bu::DiskUsage::~DiskUsage()
{}


void evb::bu::DiskUsage::update()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);

  if ( ! lock ) return; // don't start another thread if there's already one

  boost::thread thread(
    boost::bind( &DiskUsage::doStatFs, this )
  );

  valid_ = thread.timed_join( boost::posix_time::milliseconds(500) );
}


float evb::bu::DiskUsage::overThreshold()
{
  const float diskUsage = relDiskUsage();

  if ( ! valid_ ) return 1; //error condition

  if ( deleteFiles_ && diskUsage > 0.9 )
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
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
