#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"

#include <boost/bind.hpp>

evb::EvBidFactory::EvBidFactory() :
runNumber_(0),
previousEventNumber_(0),
resyncCount_(0),
fakeLumiSection_(1)
{}


evb::EvBidFactory::~EvBidFactory()
{
  stopFakeLumiThread();
}

void evb::EvBidFactory::reset
(
  const uint32_t runNumber,
  const uint32_t fakeLumiSectionDuration
)
{
  runNumber_ = runNumber;
  previousEventNumber_ = 0;
  resyncCount_ = 0;

  stopFakeLumiThread();
  fakeLumiSection_ = 1;
  if ( fakeLumiSectionDuration > 0 )
  {
    fakeLumiThread_.reset(
      new boost::thread( boost::bind( &evb::EvBidFactory::fakeLumiActivity, this,
                                      static_cast<boost::posix_time::seconds>(fakeLumiSectionDuration) ) )
    );
  }
}


void evb::EvBidFactory::stopFakeLumiThread()
{
  if ( fakeLumiThread_.get() )
  {
    fakeLumiThread_->interrupt();
    fakeLumiThread_->join();
  }
}


void evb::EvBidFactory::fakeLumiActivity(const boost::posix_time::seconds fakeLumiSectionDuration)
{
  boost::system_time nextLumiSectionStartTime = boost::get_system_time();
  while(1)
  {
    nextLumiSectionStartTime += fakeLumiSectionDuration;
    boost::this_thread::sleep(nextLumiSectionStartTime);
    ++fakeLumiSection_;
  }
}


evb::EvBid evb::EvBidFactory::getEvBid()
{
  const uint32_t fakeEventNumber = (previousEventNumber_+1) % (1 << 24);
  return getEvBid(fakeEventNumber);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  return getEvBid(eventNumber,fakeLumiSection_);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber, const uint32_t lsNumber)
{
  EvBid evbId(resyncCount_,eventNumber,lsNumber,runNumber_);

  if ( eventNumber <= previousEventNumber_ )
  {
    ++resyncCount_;
    evbId = EvBid(resyncCount_,eventNumber,lsNumber,runNumber_);
  }

  previousEventNumber_ = eventNumber;
  return evbId;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
