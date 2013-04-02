#ifndef _evb_PerformaceMonitor_h_
#define _evb_PerformaceMonitor_h_

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>


namespace evb {

  struct PerformanceMonitor
  {
    uint64_t N;
    uint64_t sumOfSizes;
    uint64_t sumOfSquares;
    double seconds;
    
    PerformanceMonitor() :
    N(0), sumOfSizes(0), sumOfSquares(0)
    {
      struct timeval time;
      gettimeofday(&time,0);
      seconds = time.tv_sec + static_cast<double>(time.tv_usec) / 1000000;
    }
    
    PerformanceMonitor& operator=(const PerformanceMonitor& other)
    {
      N = other.N;
      sumOfSizes = other.sumOfSizes;
      sumOfSquares = other.sumOfSquares;
      seconds = other.seconds;

      return *this;
    }

    const PerformanceMonitor operator-(const PerformanceMonitor& other) const
    {
      PerformanceMonitor diff;
      diff.N = N - other.N;
      diff.sumOfSizes = sumOfSizes - other.sumOfSizes;
      diff.sumOfSquares = sumOfSquares - other.sumOfSquares;
      diff.seconds = seconds - other.seconds;

      return diff;
    }
  };
  
} //namespace evb

#endif // _evb_PerformaceMonitor_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
