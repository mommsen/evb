#ifndef _evb_DumpUtility_h_
#define _evb_DumpUtility_h_

#include <stdint.h>
#include <iostream>


namespace evb {

  class DumpUtility
  {
  public:

    static void dumpBlockData
    (
      std::ostream&,
      const unsigned char* data,
      uint32_t len
    );

  }; // class DumpUtility

} // namespace evb

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
