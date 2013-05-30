#ifndef _evb_EvBidFactory_h_
#define _evb_EvBidFactory_h_

#include <stdint.h>

namespace evb {

  class EvBid;

  class EvBidFactory
  {
  public:
    
    EvBidFactory();
    
    /**
     * Return EvBid for given eventNumber
     */
    EvBid getEvBid(const uint32_t eventNumber, const uint32_t lsNumber=0);

    /**
     * Reset the counters for a new run
     */
    void reset(const uint32_t runNumber);
    
  private:
    
    uint32_t runNumber_;
    uint32_t previousEventNumber_;
    uint32_t resyncCount_;
    
  };

} // namespace evb

#endif // _evb_EvBidFactory_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
