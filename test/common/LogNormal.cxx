#include <assert.h>
#include <stdint.h>

#include "evb/Constants.h"
#include "evb/FragmentSize.h"
#include "toolbox/version.h"
#include "toolbox/math/random.h"

int main( int argc, const char* argv[] )
{
  const uint32_t fedSize = 2048;
  const uint32_t minFedSize = 16;
  const uint32_t maxFedSize = 4084;
  const uint32_t iterations = 10000000;

  {
    evb::FragmentSize fixedSize(fedSize);
    assert( fixedSize.get() == fedSize );
  }

  {
    evb::FragmentSize fixedSize(2000);
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
    std::cout << "Time per iteration with boost: " << deltaT/iterations << " ns" << std::endl;
  }

  {
    #if TOOLBOX_VERSION_MAJOR >= 9 and TOOLBOX_VERSION_MINOR >= 4
    toolbox::math::DefaultRandomEngine randomNumberEngine( evb::getTimeStamp() );
    toolbox::math::CanonicalRandomEngine<double> canonicalRandomEngine(randomNumberEngine);
    toolbox::math::LogNormalDistribution<double> logNormalDistribution(fedSize,fedSize);
    #else
    toolbox::math::LogNormalGen logNormalGen(evb::getTimeStamp(),fedSize,fedSize);
    #endif

    uint32_t startTime = evb::getTimeStamp();

    for (uint32_t i=0; i < iterations; ++i)
    {
      uint32_t size = std::max((uint32_t)
        #if TOOLBOX_VERSION_MAJOR >= 9 and TOOLBOX_VERSION_MINOR >= 4
        logNormalDistribution(canonicalRandomEngine)
        #else
        logNormalGen.getRawRandomSize()
        #endif
        ,minFedSize);
      if ( maxFedSize > 0 && size > maxFedSize ) size = maxFedSize;
      assert( size >= minFedSize );
      assert( size <= maxFedSize );
    }
    const uint32_t deltaT = evb::getTimeStamp() - startTime;
    std::cout << "Time per iteration with new toolbox: " << deltaT/iterations << " ns" << std::endl;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
