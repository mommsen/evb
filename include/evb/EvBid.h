#ifndef _evb_EvBid_h_
#define _evb_EvBid_h_

#include <iostream>
#include <list>
#include <stdint.h>

#include "i2o/i2o.h"


namespace evb {

  /**
   * The event-builder ID is the unique identifier used in the event builder
   * This element type IS and MUST be a multiple of 4 bytes.
   */
  class EvBid
  {
  public:

    EvBid()
      : resyncCount_(0),eventNumber_(0),bxId_(0),lumiSection_(0),runNumber_(0),padding_(0) {};

    EvBid(uint32_t resyncCount, uint32_t eventNumber, uint32_t bxId, uint32_t lumiSection, uint32_t runNumber)
      : resyncCount_(resyncCount),eventNumber_(eventNumber),bxId_(bxId),lumiSection_(lumiSection),runNumber_(runNumber),padding_(0) {};

    /**
     * Return the resync count
     */
    uint32_t resyncCount() const
    { return resyncCount_; }

    /**
     * Return the event number
     */
    uint32_t eventNumber() const
    { return eventNumber_; }

    /**
     * Return the bunch crossing id
     */
    uint32_t bxId() const
    { return bxId_; }

    /**
     * Return the lumi-section number
     */
    uint32_t lumiSection() const
    { return lumiSection_; }

    /**
     * Return the run number
     */
    uint32_t runNumber() const
    { return runNumber_; }

    /**
     * Return true if the EvB id is valid
     */
    bool isValid() const
    { return ( *this != EvBid() ); }

    /**
     * comparison operators
     */
    bool operator< (const EvBid&) const;
    bool operator== (const EvBid&) const;
    bool operator!= (const EvBid&) const;

  private:

    uint32_t resyncCount_; // The number of L1 trigger number resets due to resyncs
    uint32_t eventNumber_; // The L1 trigger number, aka event number
    uint32_t bxId_;        // The bunch crossing id
    uint32_t lumiSection_; // The lumi-section number. This number is optional
    uint32_t runNumber_;   // The run number
    uint32_t padding_;     // allign the EvBid to 64 bit

  };


  inline bool EvBid::operator< (const EvBid& other) const
  {
    if ( runNumber() != other.runNumber() ) return ( runNumber() < other.runNumber() );
    if ( resyncCount() != other.resyncCount() ) return ( resyncCount() < other.resyncCount() );
    return ( eventNumber() < other.eventNumber() );
  }

  inline bool EvBid::operator== (const EvBid& other) const
  {
    return (
      runNumber() == other.runNumber() &&
      resyncCount() == other.resyncCount() &&
      eventNumber() == other.eventNumber()
    );
  }

  inline bool EvBid::operator!= (const EvBid& other) const
  {
    return !( *this == other );
  }

  inline std::ostream& operator<< (std::ostream& s, const evb::EvBid& evbId)
  {
    s << "runNumber=" << evbId.runNumber() << " ";
    if ( evbId.lumiSection() > 0 )
      s << "lumiSection=" << evbId.lumiSection() << " ";
    s << "resyncCount=" << evbId.resyncCount() << " ";
    s << "eventNumber=" << evbId.eventNumber() << " ";
    s << "bxId=" << evbId.bxId();

    return s;
  }

} // namespace evb


#endif // _evb_EvBid_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
