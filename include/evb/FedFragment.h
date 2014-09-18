#ifndef _evb_FedFragment_h_
#define _evb_FedFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/Reference.h"


namespace evb {

  /**
   * \ingroup xdaqApps
   * \brief Represent a FED fragment
   */

  class FedFragment
  {
  public:

    FedFragment(toolbox::mem::Reference*, tcpla::MemoryCache* = 0);

    ~FedFragment();

    void setEvBid(const EvBid evbId) { evbId_ = evbId; }

    EvBid getEvBid() const { return evbId_; }
    uint16_t getFedId() const { return fedId_; }
    uint32_t getEventNumber() const { return eventNumber_; }
    bool isCorrupted() const { return isCorrupted_; }
    toolbox::mem::Reference* getBufRef() const { return bufRef_; }
    unsigned char* getFedPayload() const;

    struct FedErrors
    {
      uint32_t corruptedEvents;
      uint32_t crcErrors;
      uint32_t fedErrors;
      uint32_t nbDumps;

      FedErrors() { reset(); }
      void reset()
      { corruptedEvents=0;crcErrors=0;fedErrors=0;nbDumps=0; }
    };

    /**
     * Check the consistency of the FED event fragment
     */
    void checkIntegrity(uint32_t&fedSize, FedErrors&, const uint32_t checkCRC);

  private:

    uint16_t fedId_;
    uint32_t eventNumber_;
    EvBid evbId_;
    bool isCorrupted_;
    toolbox::mem::Reference* bufRef_;
    tcpla::MemoryCache* cache_;

    CRCCalculator crcCalculator_;

  };

  typedef boost::shared_ptr<FedFragment> FedFragmentPtr;

} //namespace evb


#endif // _evb_FedFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
