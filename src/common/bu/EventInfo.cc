#include <sstream>
#include <zlib.h>

#include "evb/bu/EventInfo.h"


evb::bu::EventInfo::EventInfo
(
  const uint32_t run,
  const uint32_t lumi,
  const uint32_t event
) :
  version_(3),
  runNumber_(run),
  lumiSection_(lumi),
  eventNumber_(event),
  eventSize_(0),
  paddingSize_(0),
  adler32_(::adler32(0L,Z_NULL,0))
{}


void evb::bu::EventInfo::updateAdler32(const iovec& loc)
{
  adler32_ = ::adler32(adler32_, (Bytef*)loc.iov_base, loc.iov_len);
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
      str << " adler32=0x" << std::hex << eventInfo.adler32() << std::dec;
      str << " eventSize=" << eventInfo.eventSize();
      str << " paddingSize=" << eventInfo.paddingSize();

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
