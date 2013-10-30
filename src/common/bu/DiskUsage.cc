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
previousOverThreshold_(0),
retVal_(1)
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


bool evb::bu::DiskUsage::update()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);

  if ( ! lock ) return false; // don't start another thread if there's already one

  boost::thread thread(
    boost::bind( &DiskUsage::doStatFs, this )
  );

  return (
    thread.timed_join( boost::posix_time::milliseconds(500) ) &&
    retVal_ == 0
  );
}


float evb::bu::DiskUsage::overThreshold()
{
  const float diskUsage = relDiskUsage();

  float overThreshold = (diskUsage < lowWaterMark_) ? 0 :
    (diskUsage - lowWaterMark_) / (highWaterMark_ - lowWaterMark_);

  if ( diskUsage < 0 ) overThreshold = 1; //error condition

  if ( deleteFiles_ && overThreshold > 0.95 )
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

  const float deltaOverThreshold = overThreshold - previousOverThreshold_;
  previousOverThreshold_ = overThreshold;

  return deltaOverThreshold;
}


float evb::bu::DiskUsage::diskSizeGB()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);
  if ( ! lock ) return -2;

  return ( retVal_==0 ? static_cast<float>(statfs_.f_blocks * statfs_.f_bsize) / 1024 / 1024 / 1024 : -1 );
}


float evb::bu::DiskUsage::relDiskUsage()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);
  if ( ! lock ) return -2;

  return ( retVal_==0 ? 1 - static_cast<float>(statfs_.f_bavail)/statfs_.f_blocks : -1 );
}


void evb::bu::DiskUsage::doStatFs()
{
  retVal_ = statfs64(path_.string().c_str(), &statfs_);
  if (path_ == "/aSlowDiskForUnitTests") ::sleep(5);
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
