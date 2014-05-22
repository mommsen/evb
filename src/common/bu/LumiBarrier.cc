#include <stdexcept>

#include "evb/bu/LumiBarrier.h"


evb::bu::LumiBarrier::LumiBarrier(const uint16_t threadCount) :
  threadCount_(threadCount),
  lastLumiSection_(0)
{
  if (threadCount == 0)
    throw std::invalid_argument("threadCount cannot be zero.");
}


void evb::bu::LumiBarrier::reachedLumiSection(const uint32_t lumiSection)
{
  uint32_t ls = lastLumiSection_+1;
  while ( ls <= lumiSection && ls > lastLumiSection_ )
  {
    waitForLumiSection(ls);
    ++ls;
  }
}


void evb::bu::LumiBarrier::unblockCurrentLumiSection()
{
  boost::mutex::scoped_lock lock(mutex_);
  countPerLS_.erase(lastLumiSection_);
  ++lastLumiSection_;
  cond_.notify_all();
}


void evb::bu::LumiBarrier::unblockUpToLumiSection(const uint32_t lumiSection)
{
  if ( lumiSection < lastLumiSection_ ) return;

  boost::mutex::scoped_lock lock(mutex_);
  for (uint32_t ls = lastLumiSection_; ls <= lumiSection; ++ls)
  {
    countPerLS_.erase(ls);
  }
  lastLumiSection_ = lumiSection;
  cond_.notify_all();
}


void evb::bu::LumiBarrier::waitForLumiSection(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock lock(mutex_);

  CountPerLS::iterator pos = countPerLS_.find(lumiSection);
  if ( pos == countPerLS_.end() )
  {
    pos = countPerLS_.insert(pos,CountPerLS::value_type(lumiSection,threadCount_));
  }
  if ( --(pos->second) == 0 )
  {
    countPerLS_.erase(pos);
    ++lastLumiSection_;
    cond_.notify_all();
  }
  else
  {
    do
      cond_.wait(lock);
    while ( countPerLS_.find(lumiSection) != countPerLS_.end() );
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
