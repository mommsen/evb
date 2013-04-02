#ifndef _evb_EvBid_h_
#define _evb_EvBid_h_

#include <iostream>
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
    : resyncCount_(0), eventNumber_(0) {};
    
    EvBid(uint32_t resyncCount, uint32_t eventNumber)
    : resyncCount_(resyncCount), eventNumber_(eventNumber) {};

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
     * Return true if the EvB id is valid
     */
    bool isValid() const
    { return !( resyncCount_ == 0 && eventNumber_ == 0 ); }

    /**
     * operator< induces a strict weak ordering, so that EvBid can
     * be used as a key in std::map.
     */
    bool operator< (const EvBid& other) const;
    
    /**
     * operator== returns true if both the resyncCount and eventNumber are equal.
     */
    bool operator== (const EvBid& other) const;
    
    /**
     * operator!= is the negation of operator==.
     */
    bool operator!= (const EvBid& other) const;
    
    
  private:

    uint32_t resyncCount_; // The number of L1 trigger number resets due to resyncs
    uint32_t eventNumber_; // The L1 trigger number, aka event number

  };


  inline bool EvBid::operator< (const EvBid& other) const
  {
    return resyncCount_ == other.resyncCount_
      ? eventNumber_ < other.eventNumber_
      : resyncCount_ < other.resyncCount_;
  }

  inline bool EvBid::operator== (const EvBid& other) const
  {
    return resyncCount_ == other.resyncCount_ && eventNumber_ == other.eventNumber_;
  }

  inline bool EvBid::operator!= (const EvBid& other) const
  {
    return resyncCount_ != other.resyncCount_ || eventNumber_ != other.eventNumber_;
  }

} // namespace evb


inline std::ostream& operator<<
(
  std::ostream& s,
  const evb::EvBid& evbId
)
{
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
