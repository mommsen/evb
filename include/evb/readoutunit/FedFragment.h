#ifndef _evb_readoutunit_FedFragment_h_
#define _evb_readoutunit_FedFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/Reference.h"

#include "evb/readoutunit/ferolTCP/BufferPool.h"

namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Represent a FED fragment
     */

    class FedFragment
    {
    public:

      FedFragment(toolbox::mem::Reference*, tcpla::MemoryCache*);
      FedFragment(uint16_t fedId, const EvBid&, toolbox::mem::Reference*);
      FedFragment(uint16_t fedId, uint32_t eventNumber, unsigned char* payload, size_t length, ferolTCP::Buffer*& ferolTCPBuffer);

      ~FedFragment();

      void setEvBid(const EvBid& evbId) { evbId_ = evbId; }

      EvBid getEvBid() const { return evbId_; }
      uint16_t getFedId() const { return fedId_; }
      uint32_t getEventNumber() const { return eventNumber_; }
      bool isCorrupted() const { return isCorrupted_; }
      unsigned char* getPayload() const { return payload_; }
      unsigned char* getFedPayload() const;
      size_t getLength() const { return length_; }
      uint32_t getFedSize();
      void dump(std::ostream&, const std::string& reasonForDump);

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

      void checkIntegrity(FedErrors&, const uint32_t checkCRC);


    private:

      void calculateSize();
      std::string trailerBitToString(const uint32_t conscheck) const;

      uint16_t fedId_;
      uint32_t eventNumber_;
      EvBid evbId_;
      uint32_t fedSize_;
      bool isCorrupted_;
      unsigned char* payload_;
      size_t length_;
      toolbox::mem::Reference* bufRef_;
      tcpla::MemoryCache* cache_;

      static CRCCalculator crcCalculator_;

      ferolTCP::Buffer* ferolTCPBuffer_;

    };

    typedef boost::shared_ptr<FedFragment> FedFragmentPtr;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_FedFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
