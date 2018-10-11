#ifndef _evb_readoutunit_FedFragment_h_
#define _evb_readoutunit_FedFragment_h_

#include <memory>
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
        const std::string& subSystem,
        const EvBidFactoryPtr&,
        const uint32_t checkCRC,
        uint32_t* fedErrorCount,
        uint32_t* crcErrors
      );

      FedFragment
      (
        const uint16_t fedId,
        const bool isMasterFed,
        const std::string& subSystem,
        const EvBid&,
        toolbox::mem::Reference*
      );

      ~FedFragment();

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

      virtual bool fillData(unsigned char* payload, const uint32_t remainingPayloadSize, uint32_t& copiedSize);


    protected:

      enum FedComponent
      {
        FEROL_HEADER,
        FED_HEADER,
        FED_PAYLOAD,
        FED_TRAILER
      };
      FedComponent typeOfNextComponent_;

      const uint16_t fedId_;
      uint16_t bxId_;
      uint32_t eventNumber_;
      uint32_t fedSize_;
      EvBid evbId_;
      bool isComplete_;

      uint32_t tmpBufferSize_;
      std::vector<unsigned char> tmpBuffer_;

      const EvBidFactoryPtr evbIdFactory_;
      static CRCCalculator crcCalculator_;


    private:

      bool parse(toolbox::mem::Reference*, uint32_t& usedSize);
      void checkFerolHeader(const ferolh_t*);
      void checkFedHeader(const fedh_t*);
      void checkFedTrailer(fedt_t*);
      void checkCRC(fedt_t*);
      void checkTrailerBits(const uint32_t conscheck);
      void reportErrors() const;

      const uint32_t checkCRC_;
      uint32_t* fedErrorCount_;
      uint32_t* crcErrors_;
      const bool isMasterFed_;
      const std::string& subSystem_;
      std::string errorMsg_;
      bool isCorrupted_;
      bool isOutOfSequence_;
      bool hasCRCerror_;
      bool hasFEDerror_;
      toolbox::mem::Reference* bufRef_;
      using SocketBuffers = std::vector<SocketBufferPtr>;
      SocketBuffers socketBuffers_;
      DataLocations dataLocations_;
      DataLocations::const_iterator copyIterator_;
      uint32_t copyOffset_;

      bool isLastFerolHeader_;
      uint32_t payloadLength_;
    };

    using FedFragmentPtr = std::shared_ptr<FedFragment>;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_FedFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
