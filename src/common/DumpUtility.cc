#include "evb/DumpUtility.h"
#include "toolbox/string.h"


void evb::DumpUtility::dumpBlockData
(
  std::ostream& s,
  const unsigned char* data,
  uint32_t len
)
{
  uint32_t* d = (uint32_t*)data;
  len /= 4;

  for (uint32_t ic=0; ic<len; ic=ic+4)
  {
    // avoid to write beyond the buffer:
    if (ic + 2 >= len)
    {
      s << toolbox::toString("%08x : %08x %08x %12s         |    human readable swapped : %08x %08x %12s      : %08x", ic*4, d[ic], d[ic+1], "", d[ic+1], d[ic], "", ic*4);
      s << std::endl;
    }
    else
    {
      s << toolbox::toString("%08x : %08x %08x %08x %08x    |    human readable swapped : %08x %08x %08x %08x : %08x", ic*4, d[ic], d[ic+1], d[ic+2], d[ic+3],  d[ic+1], d[ic], d[ic+3], d[ic+2], ic*4);
      s << std::endl;
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
