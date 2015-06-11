#ifndef _evb_readoutunit_FedFragment_h_
#define _evb_readoutunit_FedFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <sys/uio.h>
#include <vector>

#include "evb/EvBid.h"
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

      FedFragment();
      ~FedFragment();

      void append(toolbox::mem::Reference*, tcpla::MemoryCache*);
      void append(uint16_t fedId, const EvBid&, toolbox::mem::Reference*);
      void append(SocketBufferPtr&, uint32_t& usedSize);

      void setEvBid(const EvBid& evbId) { evbId_ = evbId; }

      EvBid getEvBid() const { return evbId_; }
      uint16_t getFedId() const { return fedId_; }
      uint32_t getEventNumber() const { return eventNumber_; }
      bool isCorrupted() const { return isCorrupted_; }
      bool isComplete() const { return isComplete_; }
      toolbox::mem::Reference* getBufRef() const { return bufRef_; }
      uint32_t getFedSize() const { return fedSize_; }
      void dump(std::ostream&, const std::string& reasonForDump);

      typedef std::vector<iovec> DataLocations;
      const DataLocations& getDataLocations() const { return dataLocations_; }


    private:

      void parse(toolbox::mem::Reference*, uint32_t& usedSize);
      uint32_t checkFerolHeader(const ferolh_t*);
      void checkFedHeader(const fedh_t*);
      void checkFedTrailer(const fedt_t*);
      std::string trailerBitToString(const uint32_t conscheck) const;

      enum FedComponent
      {
        FEROL_HEADER,
        FED_HEADER,
        FED_PAYLOAD,
        FED_TRAILER
      };
      FedComponent typeOfNextComponent_;

      uint16_t fedId_;
      uint32_t eventNumber_;
      EvBid evbId_;
      uint32_t fedSize_;
      bool isCorrupted_;
      bool isComplete_;
      toolbox::mem::Reference* bufRef_;
      tcpla::MemoryCache* cache_;
      std::vector<SocketBufferPtr> socketBuffers_;
      DataLocations dataLocations_;

      bool isLastFerolHeader_;
      uint32_t payloadLength_;

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
