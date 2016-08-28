#ifndef _evb_PerformaceMonitor_h_
#define _evb_PerformaceMonitor_h_

#include "evb/Constants.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>


namespace evb {

  struct PerformanceMonitor
  {
    uint64_t logicalCount;
    uint64_t i2oCount;
    uint64_t retryCount;
    uint64_t sumOfSizes;
    uint64_t sumOfSquares;
    uint64_t startTime;

    PerformanceMonitor()
    {
      reset();
    }

    double deltaT() const
    {
      const uint64_t now = getTimeStamp();
      return (now>startTime ? (now-startTime)/1e9 : 0);
    }

    double logicalRate(const double& deltaT) const
    {
      return ( deltaT>0 ? logicalCount/deltaT : 0 );
    }

    double i2oRate(const double& deltaT) const
    {
      return ( deltaT>0 ? i2oCount/deltaT : 0 );
    }

    double retryRate(const double& deltaT) const
    {
      return ( deltaT>0 ? retryCount/deltaT : 0 );
    }

    double packingFactor() const
    {
      return ( i2oCount>0 ? static_cast<double>(logicalCount)/i2oCount : 0 );
    }

    double throughput(const double& deltaT) const
    {
      return ( deltaT>0 ? sumOfSizes/deltaT : 0 );
    }

    double throughputStdDev(const double& deltaT) const
    {
      if ( deltaT <= 0 ) return 0;

      const double meanOfSquares = sumOfSquares/deltaT;
      const double mean = sumOfSizes/deltaT;
      const double variance = meanOfSquares - (mean*mean);

      return ( variance>0 ? sqrt(variance) : 0 );
    }

    double size() const
    {
      return ( logicalCount>0 ? static_cast<double>(sumOfSizes)/logicalCount : 0 );
    }

    double sizeStdDev() const
    {
      if ( logicalCount == 0 ) return 0;

      const double meanOfSquares = static_cast<double>(sumOfSquares)/logicalCount;
      const double mean = static_cast<double>(sumOfSizes)/logicalCount;
      const double variance = meanOfSquares - (mean*mean);

      return ( variance>0 ? sqrt(variance) : 0 );
    }

    void reset()
    {
      logicalCount = 0;
      i2oCount = 0;
      retryCount = 0;
      sumOfSizes = 0;
      sumOfSquares = 0;
      startTime = getTimeStamp();
    }

    PerformanceMonitor& operator=(const PerformanceMonitor& other)
    {
      logicalCount = other.logicalCount;
      i2oCount = other.i2oCount;
      retryCount = other.retryCount;
      sumOfSizes = other.sumOfSizes;
      sumOfSquares = other.sumOfSquares;
      startTime = other.startTime;

      return *this;
    }

    const PerformanceMonitor operator-(const PerformanceMonitor& other) const
    {
      PerformanceMonitor diff;
      diff.logicalCount = logicalCount - other.logicalCount;
      diff.i2oCount = i2oCount - other.i2oCount;
      diff.retryCount = retryCount - other.retryCount;
      diff.sumOfSizes = sumOfSizes - other.sumOfSizes;
      diff.sumOfSquares = sumOfSquares - other.sumOfSquares;
      diff.startTime = startTime - other.startTime;

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
