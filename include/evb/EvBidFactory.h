#ifndef _evb_EvBidFactory_h_
#define _evb_EvBidFactory_h_

#include <stdint.h>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace evb {

  class EvBid;

  class EvBidFactory
  {
  public:

    EvBidFactory();

    /**
     * Return EvBid with a fake eventNumber and lumi section
     */
    EvBid getEvBid();

    /**
     * Return EvBid for given eventNumber and a fake lumi section
     */
    EvBid getEvBid(const uint32_t eventNumber);

    /**
     * Return EvBid for given eventNumber and lumi section
     */
    EvBid getEvBid(const uint32_t eventNumber, const uint32_t lsNumber);

    /**
     * Reset the counters for a new run
     */
    void reset(const uint32_t runNumber, const uint32_t fakeLumiSectionDuration);

  private:

    uint32_t runNumber_;
    uint32_t previousEventNumber_;
    uint32_t resyncCount_;
    uint32_t fakeLumiSection_;
    boost::posix_time::time_duration fakeLumiSectionDuration_;
    boost::posix_time::ptime startOfLumiSection_;

  };

} // namespace evb

#endif // _evb_EvBidFactory_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
