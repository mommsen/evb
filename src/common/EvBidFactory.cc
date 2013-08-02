#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"


evb::EvBidFactory::EvBidFactory() :
runNumber_(0),
previousEventNumber_(0),
resyncCount_(0),
fakeLumiSection_(1)
{}


void evb::EvBidFactory::reset(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  previousEventNumber_ = 0;
  resyncCount_ = 0;
  fakeLumiSection_ = 1;
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  if ( eventNumber%10000 == 0 ) ++fakeLumiSection_;
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
