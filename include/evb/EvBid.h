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
      : runNumber_(0),resyncCount_(0),dataWord1_(0),dataWord2_(0) {};

    EvBid
    (
      const bool resynced,
      const uint32_t resyncCount,
      const uint32_t eventNumber,
      const uint16_t bxId,
      const uint32_t lumiSection,
      const uint32_t runNumber
    )
      : runNumber_(runNumber),resyncCount_((resynced<<31)|(resyncCount&0x7fffffff)),
        dataWord1_(((bxId&0x0ff)<<24)|(eventNumber&0xffffff)),
        dataWord2_(((bxId&0xf00)<<20)|(lumiSection&0xfffffff)) {}

    /**
     * Return the resync count
     */
    uint32_t resyncCount() const
    { return (resyncCount_&0x7fffffff); }

    /**
     * Return the event number
     */
    uint32_t eventNumber() const
    { return (dataWord1_ & 0xffffff); }

    /**
     * Return the bunch crossing id
     */
    uint16_t bxId() const
    { return (((dataWord1_&0xff000000) >> 24)|((dataWord2_&0xf0000000) >> 20)); }

    /**
     * Return the lumi-section number
     */
    uint32_t lumiSection() const
    { return (dataWord2_ & 0xfffffff); }

    /**
     * Return the run number
     */
    uint32_t runNumber() const
    { return runNumber_; }

    /**
     * Return true if this event is the first after a resync
     */
    bool resynced() const
    { return ((resyncCount_&0x80000000) != 0); }

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

    uint32_t runNumber_;   // The run number
    uint32_t resyncCount_; // The number of L1 trigger number resets due to resyncs
    uint32_t dataWord1_;   // contains the lumi-section number (28 bits),
    uint32_t dataWord2_;   // the bunch crossing id (12 bits),
                           // and the L1 trigger number, aka event number (24 bits)
                           // Note that using uint64_t with shift operations is undefined
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
    if ( evbId.resynced() ) s << "*";
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
