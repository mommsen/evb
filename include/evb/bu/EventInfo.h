#ifndef _evb_bu_EventInfo_h_
#define _evb_bu_EventInfo_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <sys/uio.h>

#include "evb/CRCCalculator.h"


namespace evb {
  namespace bu {

    class EventInfo
    {
    public:

      EventInfo(
        const uint32_t runNumber,
        const uint32_t lumiSection,
        const uint32_t eventNumber
      );

      void addFedSize(const uint32_t size) { eventSize_ += size; }
      void updateCRC32(const iovec&);

      uint32_t version() const { return version_; }
      uint32_t eventNumber() const { return eventNumber_; }
      uint32_t lumiSection() const { return lumiSection_; }
      uint32_t runNumber() const { return runNumber_; }
      uint32_t eventSize() const { return eventSize_; }
      uint32_t crc32c() const { return crc32c_; }

    private:

      const uint32_t version_;
      const uint32_t runNumber_;
      const uint32_t lumiSection_;
      const uint32_t eventNumber_;
      uint32_t eventSize_;
      uint32_t crc32c_;

      static CRCCalculator crcCalculator_;

    }; // EventInfo

    typedef boost::shared_ptr<EventInfo> EventInfoPtr;

  } } // namespace evb::bu


#endif // _evb_bu_EventInfo_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
