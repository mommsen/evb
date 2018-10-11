#ifndef _evb_EvBidFactory_h_
#define _evb_EvBidFactory_h_

#include <memory>
#include <stdint.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "evb/DataLocations.h"


namespace evb {

  class EvBid;

  class EvBidFactory
  {
  public:

    EvBidFactory();
    ~EvBidFactory();

    /**
     * Pass the function to extract the lumi section information from the payload
     */
    using LumiSectionFunction = boost::function< uint32_t(const DataLocations&) >;
    void setLumiSectionFunction(LumiSectionFunction&);

    /**
     * Set the duration of the fake lumi section in seconds.
     * Setting it to 0 disables the generation of fake lumi sections.
     */
    void setFakeLumiSectionDuration(const uint32_t duration);

    /**
     * Create a resync at the given event number
     */
    void resyncAtEvent(const uint32_t eventNumber);

    /**
     * Return EvBid with a fake eventNumber, bunch crossing id, and lumi section
     */
    EvBid getEvBid();

    /**
     * Return EvBid for given eventNumber, bunch crossing id, and a fake lumi section
     */
    EvBid getEvBid(const uint32_t eventNumber, const uint16_t bxId);

    /**
     * Return EvBid for given eventNumber, bunch crossing id, and lumi section
     */
    EvBid getEvBid(const uint32_t eventNumber, const uint16_t bxId, const uint32_t lumiSection);

    /**
     * Return EvBid for given eventNumber, bunch crossing id, and extract lumi section from payload
     */
    EvBid getEvBid(const uint32_t eventNumber, const uint16_t bxId, const DataLocations&);

    /**
     * Reset the counters for a new run
     */
    void reset(const uint32_t runNumber);

  private:

    void stopFakeLumiThread();
    void fakeLumiActivity();

    uint32_t runNumber_;
    uint32_t previousEventNumber_;
    uint32_t resyncCount_;
    uint32_t resyncAtEventNumber_;
    LumiSectionFunction lumiSectionFunction_;
    boost::posix_time::seconds fakeLumiSectionDuration_;
    uint32_t fakeLumiSection_;
    std::shared_ptr<boost::thread> fakeLumiThread_;
    volatile bool doFakeLumiSections_;

  };

  using EvBidFactoryPtr = std::shared_ptr<EvBidFactory>;

} // namespace evb

#endif // _evb_EvBidFactory_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
