#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"


evb::EvBidFactory::EvBidFactory() :
runNumber_(0),
previousEventNumber_(0),
resyncCount_(0),
fakeLumiSection_(1)
{}


void evb::EvBidFactory::reset
(
  const uint32_t runNumber,
  const uint32_t fakeLumiSectionDuration
)
{
  runNumber_ = runNumber;
  previousEventNumber_ = 0;
  resyncCount_ = 0;
  fakeLumiSection_ = 1;
  fakeLumiSectionDuration_ = boost::posix_time::seconds(fakeLumiSectionDuration);
}


evb::EvBid evb::EvBidFactory::getEvBid()
{
  const uint32_t fakeEventNumber = (previousEventNumber_+1) % (1 << 24);
  return getEvBid(fakeEventNumber);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

  if ( startOfLumiSection_ == boost::posix_time::not_a_date_time )
    startOfLumiSection_ = now;

  while ( startOfLumiSection_ + fakeLumiSectionDuration_ < now )
  {
    startOfLumiSection_ += fakeLumiSectionDuration_;
    ++fakeLumiSection_;
  }

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
