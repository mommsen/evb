#ifndef _evb_readoutunit_FedFragment_h_
#define _evb_readoutunit_FedFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string>
#include <sys/uio.h>
#include <vector>

#include "evb/CRCCalculator.h"
#include "evb/DataLocations.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/readoutunit/SocketBuffer.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Represent a FED fragment
     */

    class FedFragment
    {
    public:

      FedFragment
      (
        const uint16_t fedId,
        const bool isMasterFed,
        const EvBidFactoryPtr&,
        const uint32_t checkCRC,
        uint32_t& fedErrorCount,
        uint32_t& crcErrors
      );
      ~FedFragment();

      bool append(toolbox::mem::Reference*, tcpla::MemoryCache*);
      bool append(const EvBid&, toolbox::mem::Reference*);
      bool append(SocketBufferPtr&, uint32_t& usedSize);

      EvBid getEvBid() const { return evbId_; }
      uint16_t getFedId() const { return fedId_; }
      uint32_t getEventNumber() const { return eventNumber_; }
      bool isMasterFed() const  { return isMasterFed_; }
      bool isCorrupted() const { return isCorrupted_; }
      bool isOutOfSequence() const { return isOutOfSequence_; }
      bool isComplete() const { return isComplete_; }
      toolbox::mem::Reference* getBufRef() const { return bufRef_; }
      uint32_t getFedSize() const { return fedSize_; }
      void dump(std::ostream&, const std::string& reasonForDump);

      const DataLocations& getDataLocations() const { return dataLocations_; }


    private:

      bool parse(toolbox::mem::Reference*, uint32_t& usedSize);
      void checkFerolHeader(const ferolh_t*);
      void checkFedHeader(const fedh_t*);
      void checkFedTrailer(fedt_t*);
      void checkCRC(fedt_t*);
      void checkTrailerBits(const uint32_t conscheck);
      void reportErrors() const;

      enum FedComponent
      {
        FEROL_HEADER,
        FED_HEADER,
        FED_PAYLOAD,
        FED_TRAILER
      };
      FedComponent typeOfNextComponent_;

      const EvBidFactoryPtr evbIdFactory_;
      const uint32_t checkCRC_;
      uint32_t& fedErrorCount_;
      uint32_t& crcErrors_;
      const uint16_t fedId_;
      const bool isMasterFed_;
      uint16_t bxId_;
      uint32_t eventNumber_;
      EvBid evbId_;
      uint32_t fedSize_;
      std::string errorMsg_;
      bool isCorrupted_;
      bool isOutOfSequence_;
      bool hasCRCerror_;
      bool hasFEDerror_;
      bool isComplete_;
      toolbox::mem::Reference* bufRef_;
      tcpla::MemoryCache* cache_;
      typedef std::vector<SocketBufferPtr> SocketBuffers;
      SocketBuffers socketBuffers_;
      DataLocations dataLocations_;

      bool isLastFerolHeader_;
      uint32_t payloadLength_;

      uint32_t tmpBufferSize_;
      std::vector<unsigned char> tmpBuffer_;

      static CRCCalculator crcCalculator_;
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
