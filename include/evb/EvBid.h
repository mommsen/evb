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
    : resyncCount_(0), eventNumber_(0), lumiSection_(0), runNumber_(0) {};
    
    EvBid(uint32_t resyncCount, uint32_t eventNumber, uint32_t lumiSection, uint32_t runNumber)
    : resyncCount_(resyncCount), eventNumber_(eventNumber), lumiSection_(lumiSection), runNumber_(runNumber) {};

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
    bool operator< (const EvBid& other) const;
    bool operator== (const EvBid& other) const;
    bool operator!= (const EvBid& other) const;
     
    
  private:

    uint32_t resyncCount_; // The number of L1 trigger number resets due to resyncs
    uint32_t eventNumber_; // The L1 trigger number, aka event number
    uint32_t lumiSection_;    // The lumi-section number. This number is optional
    uint32_t runNumber_;   // The run number

  };


  inline bool EvBid::operator< (const EvBid& other) const
  {
    if ( runNumber_ != other.runNumber_ ) return ( runNumber_ < other.runNumber_ );
    if ( resyncCount_ != other.resyncCount_ ) return ( resyncCount_ < other.resyncCount_ );
    return ( eventNumber_ < other.eventNumber_ );
  }

  inline bool EvBid::operator== (const EvBid& other) const
  {
    return (
      runNumber_ == other.runNumber_ &&
      resyncCount_ == other.resyncCount_ &&
      eventNumber_ == other.eventNumber_
    );
  }

  inline bool EvBid::operator!= (const EvBid& other) const
  {
    return !( *this == other );
  }
  
  typedef std::list<EvBid> EvBids;
  
} // namespace evb


inline std::ostream& operator<<
(
  std::ostream& s,
  const evb::EvBid evbId
)
{
  s << "runNumber=" << evbId.runNumber() << " ";
  s << "lumiSection=" << evbId.lumiSection() << " ";
  s << "resyncCount=" << evbId.resyncCount() << " ";
  s << "eventNumber=" << evbId.eventNumber();
  
  return s;
}


#endif // _evb_EvBid_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
