#ifndef _evb_readoutunit_DummyFragment_h_
#define _evb_readoutunit_DummyFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string>

#include "evb/EvBidFactory.h"
#include "evb/readoutunit/FedFragment.h"


namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Represent a dummy FED fragment
     */

    class DummyFragment : public FedFragment
    {
    public:

      DummyFragment
      (
        const uint16_t fedId,
        const bool isMasterFed,
        const uint32_t fedSize,
        const bool computeCRC,
        const std::string& subSystem,
        const EvBidFactoryPtr&,
        const uint32_t checkCRC,
        uint32_t& fedErrorCount,
        uint32_t& crcErrors
      );
      ~DummyFragment();

      virtual bool fillData(unsigned char* payload, const uint32_t remainingPayloadSize, uint32_t& copiedSize);

    private:

      void fillFedHeader(fedh_t*);
      void fillFedTrailer(fedt_t*);

      uint32_t remainingFedSize_;
      const bool computeCRC_;
      uint16_t fedCRC_;

    };

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_DummyFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
