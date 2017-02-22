#include <assert.h>
#include <stdint.h>

#include <boost/math/distributions/lognormal.hpp>
#include <boost/random.hpp>

#include "evb/Constants.h"
#include "evb/FragmentSize.h"
#include "toolbox/math/random.h"
#include "toolbox/math/random2.h"

int main( int argc, const char* argv[] )
{
  const uint32_t fedSize = 2048;
  const uint32_t minFedSize = 16;
  const uint32_t maxFedSize = 4084;
  const uint32_t iterations = 10000000;

  {
    evb::FragmentSize fixedSize(fedSize,0,minFedSize,maxFedSize);
    assert( fixedSize.get() == fedSize );
  }

  {
    evb::FragmentSize fixedSize(2000,0,minFedSize,maxFedSize);
    assert( fixedSize.get() == (2000 & ~0x7) );
  }

  {
    evb::FragmentSize varSize(fedSize,fedSize,minFedSize,maxFedSize);

    uint32_t startTime = evb::getTimeStamp();

    for (uint32_t i=0; i < iterations; ++i)
    {
      const uint32_t size = varSize.get();
      assert( size >= minFedSize );
      assert( size <= maxFedSize );
    }
    const uint32_t deltaT = evb::getTimeStamp() - startTime;
    std::cout << "Time per iteration with FragmentSize: " << deltaT/iterations << " ns" << std::endl;
  }

  {
    typedef boost::rand48 RNG;
    //typedef boost::mt19937 RNG;
    RNG rng( evb::getTimeStamp() );
    boost::lognormal_distribution<> lnd(fedSize,fedSize);
    boost::variate_generator< RNG,boost::lognormal_distribution<> > logNormalGenerator(rng,lnd);

    uint32_t startTime = evb::getTimeStamp();

    for (uint32_t i=0; i < iterations; ++i)
    {
      uint32_t size = std::max(minFedSize,(uint32_t)logNormalGenerator());
      if ( maxFedSize > 0 && size > maxFedSize ) size = maxFedSize;
      assert( size >= minFedSize );
      assert( size <= maxFedSize );
    }
    const uint32_t deltaT = evb::getTimeStamp() - startTime;
    std::cout << "Time per iteration with boost: " << deltaT/iterations << " ns" << std::endl;
  }

  {
    toolbox::math::LogNormalGen logNormalGen(evb::getTimeStamp(),fedSize,fedSize);

    uint32_t startTime = evb::getTimeStamp();

    for (uint32_t i=0; i < iterations; ++i)
    {
      uint32_t size = std::max(minFedSize,(uint32_t)logNormalGen.getRawRandomSize());
      if ( maxFedSize > 0 && size > maxFedSize ) size = maxFedSize;
      assert( size >= minFedSize );
      assert( size <= maxFedSize );
    }
    const uint32_t deltaT = evb::getTimeStamp() - startTime;
    std::cout << "Time per iteration with LogNormalGen: " << deltaT/iterations << " ns" << std::endl;
  }

  {
    toolbox::math::DefaultRandomEngine randomNumberEngine( evb::getTimeStamp() );
    toolbox::math::CanonicalRandomEngine<double> canonicalRandomEngine(randomNumberEngine);
    toolbox::math::LogNormalRealDistribution<double> logNormalDistribution(fedSize,fedSize);

    uint32_t startTime = evb::getTimeStamp();

    for (uint32_t i=0; i < iterations; ++i)
    {
      uint32_t size = std::max(minFedSize,(uint32_t)logNormalDistribution(canonicalRandomEngine));
      if ( maxFedSize > 0 && size > maxFedSize ) size = maxFedSize;
      assert( size >= minFedSize );
      assert( size <= maxFedSize );
    }
    const uint32_t deltaT = evb::getTimeStamp() - startTime;
    std::cout << "Time per iteration with LogNormalRealDistribution: " << deltaT/iterations << " ns" << std::endl;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
