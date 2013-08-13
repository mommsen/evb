#include <iostream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread.hpp>

#include "evb/bu/DiskUsage.h"


evb::bu::DiskUsage::DiskUsage
(
  const boost::filesystem::path& path,
  const double lowWaterMark,
  const double highWaterMark
) :
path_(path),
lowWaterMark_(lowWaterMark),
highWaterMark_(highWaterMark),
retVal_(1),
tooHigh_(true)
{
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


bool evb::bu::DiskUsage::tooHigh()
{
  const double diskUsage = relDiskUsage();

  if ( diskUsage < 0 ) return true; //error condition

  // The disk usage is too high if the high water mark is exceeded.
  // If the disk usage exceeded the high water mark, the usage has to decrease
  // to the low water mark before setting tooHigh to false again.
  if ( tooHigh_ )
    tooHigh_ = ( diskUsage > lowWaterMark_ );
  else
    tooHigh_ = ( diskUsage > highWaterMark_ );

  return tooHigh_;
}


double evb::bu::DiskUsage::diskSizeGB()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);
  if ( ! lock ) return -2;

  return ( retVal_==0 ? static_cast<double>(statfs_.f_blocks * statfs_.f_bsize) / 1024 / 1024 / 1024 : -1 );
}


double evb::bu::DiskUsage::relDiskUsage()
{
  boost::mutex::scoped_lock lock(mutex_, boost::try_to_lock);
  if ( ! lock ) return -2;

  return ( retVal_==0 ? 1 - static_cast<double>(statfs_.f_bavail)/statfs_.f_blocks : -1 );
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
