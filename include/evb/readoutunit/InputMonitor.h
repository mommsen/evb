#ifndef _evb_readoutunit_InputMonitor_h_
#define _evb_readoutunit_InputMonitor_h_

#include <stdint.h>

#include "evb/PerformanceMonitor.h"


namespace evb {

  namespace readoutunit {

    struct InputMonitor
    {
      uint32_t lastEventNumber;
      uint64_t eventCount;
      PerformanceMonitor perf;
      double rate;
      double eventSize;
      double eventSizeStdDev;
      double bandwidth;

      InputMonitor() { reset(); }

      void reset()
      { lastEventNumber=0;eventCount=0;rate=0;eventSize=0;eventSizeStdDev=0;bandwidth=0; }
    };

  } } // namespace evb::readoutunit


#endif // _evb_readoutunit_InputMonitor_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
