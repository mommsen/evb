#include <sstream>
#include <sys/uio.h>

#include "evb/bu/EventInfo.h"


evb::CRCCalculator evb::bu::EventInfo::crcCalculator_;


evb::bu::EventInfo::EventInfo
(
  const uint32_t run,
  const uint32_t lumi,
  const uint32_t event
) :
  version_(5),
  runNumber_(run),
  lumiSection_(lumi),
  eventNumber_(event),
  eventSize_(0),
  crc32c_(0)
{}


void evb::bu::EventInfo::updateCRC32(const iovec& loc)
{
  crc32c_ = crcCalculator_.crc32c(crc32c_, (const unsigned char*)loc.iov_base, loc.iov_len);
}


namespace evb{
  namespace bu {

    std::ostream& operator<<
    (
      std::ostream& str,
      const evb::bu::EventInfo& eventInfo
    )
    {
      str << "EventInfo:";
      str << " version=" << eventInfo.version();
      str << " runNumber=" << eventInfo.runNumber();
      str << " lumiSection=" << eventInfo.lumiSection();
      str << " eventNumber=" << eventInfo.eventNumber();
      str << " crc32c=0x" << std::hex << eventInfo.crc32c() << std::dec;
      str << " eventSize=" << eventInfo.eventSize();

      return str;
    }

  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
