#ifndef _evb_DumpUtility_h_
#define _evb_DumpUtility_h_

#include <stdint.h>
#include <iostream>

#include "toolbox/mem/Reference.h"


namespace evb {

  class DumpUtility
  {
  public:

    static void dump
    (
      std::ostream&,
      const std::string& reasonForDump,
      toolbox::mem::Reference*
    );

    static void dumpBlockData
    (
      std::ostream&,
      const unsigned char* data,
      uint32_t len
    );

  private:

    static void dumpBlock
    (
      std::ostream&,
      toolbox::mem::Reference*,
      const uint32_t bufferCnt
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
