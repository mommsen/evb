#ifndef _evb_bu_FedInfo_h_
#define _evb_bu_FedInfo_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <sys/uio.h>
#include <vector>

#include "evb/CRCCalculator.h"
#include "evb/DataLocations.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"


namespace evb {
  namespace bu {

    class FedInfo
    {
    public:
      FedInfo(const unsigned char* pos, uint32_t& remainingLength);
      void addDataChunk(const unsigned char* pos, uint32_t& remainingLength);
      void checkData(const uint32_t eventNumber, const bool computeCRC) const;

      bool complete() const { return (remainingFedSize_ == 0); }

      uint32_t eventId()  const { return (header_?FED_LVL1_EXTRACT(header_->eventid):0); }
      uint16_t fedId()    const { return (header_?FED_SOID_EXTRACT(header_->sourceid):0); }
      uint32_t fedSize()  const { return FED_EVSZ_EXTRACT(trailer_->eventsize)<<3; }
      uint16_t crc()      const { return FED_CRCS_EXTRACT(trailer_->conscheck); }

    private:
      fedh_t* header_;
      fedt_t* trailer_;

      DataLocations fedData_;
      uint32_t remainingFedSize_;

      static CRCCalculator crcCalculator_;
    };

    typedef boost::shared_ptr<FedInfo> FedInfoPtr;

  } } // namespace evb::bu

#endif // _evb_bu_FedInfo_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
