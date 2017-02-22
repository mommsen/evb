#ifndef _evb_FragmentSize_h_
#define _evb_FragmentSize_h_

#include <boost/scoped_ptr.hpp>

//#define EVB_USE_RND_BOOST
#ifdef EVB_USE_RND_BOOST
#include <boost/math/distributions/lognormal.hpp>
#include <boost/random.hpp>
#else
#include "toolbox/math/random.h"
#endif


namespace evb { // namespace evb

  /**
   * Get a dummy FED fragment size
   */
  class FragmentSize
  {
  public:

    FragmentSize(
      const uint32_t meanFedSize,
      const uint32_t stdDevFedSize,
      const uint32_t minFedSize,
      const uint32_t maxFedSize
    );

    uint32_t get() const;

  private:

    const uint32_t fedSize_;
    const uint32_t minFedSize_;
    const uint32_t maxFedSize_;

    #ifdef EVB_USE_RND_BOOST
    typedef boost::rand48 RNG;
    //typedef boost::mt19937 RNG;
    typedef boost::variate_generator< RNG,boost::lognormal_distribution<> > LogNormalGenerator;
    #else
    typedef toolbox::math::LogNormalGen LogNormalGenerator;
    #endif
    boost::scoped_ptr<LogNormalGenerator> logNormalGenerator_;

  };

} // namespace evb

#endif // _evb_FragmentSize_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
