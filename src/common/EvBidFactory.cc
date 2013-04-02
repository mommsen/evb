#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"


evb::EvBidFactory::EvBidFactory() :
previousEventNumber_(0),
resyncCount_(0)
{}


void evb::EvBidFactory::reset()
{
  previousEventNumber_ = 0;
  resyncCount_ = 0;
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  EvBid evbId(resyncCount_,eventNumber);

  if ( eventNumber <= previousEventNumber_ )
  {
    ++resyncCount_;
    evbId = EvBid(resyncCount_,eventNumber);
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
