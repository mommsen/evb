#include "evb/Constants.h"
#include "evb/FragmentSize.h"


evb::FragmentSize::FragmentSize(const uint32_t fedSize) :
  fedSize_(fedSize),
  minFedSize_(0),
  maxFedSize_(0)
{}


evb::FragmentSize::FragmentSize(
  const uint32_t meanFedSize,
  const uint32_t stdDevFedSize,
  const uint32_t minFedSize,
  const uint32_t maxFedSize
) :
  fedSize_(meanFedSize),
  minFedSize_(minFedSize),
  maxFedSize_(maxFedSize)
{
  #ifdef EVB_USE_RND_BOOST
  RNG rng( getTimeStamp() );
  boost::lognormal_distribution<> lnd(meanFedSize,stdDevFedSize);
  logNormalGenerator_.reset( new LogNormalGenerator(rng,lnd) );
  #else
  logNormalGenerator_.reset( new toolbox::math::LogNormalGen(getTimeStamp(),meanFedSize,stdDevFedSize) );
  #endif
}


uint32_t evb::FragmentSize::get() const
{
  if ( logNormalGenerator_ )
  {
    uint32_t fedSize = std::max(minFedSize_,(uint32_t)
    #ifdef EVB_USE_RND_BOOST
    (*logNormalGenerator_)());
    #else
    logNormalGenerator_->getRawRandomSize());
    #endif
    if ( maxFedSize_ > 0 && fedSize > maxFedSize_ ) fedSize = maxFedSize_;
    return fedSize & ~0x7;
  }
  else
  {
    return fedSize_ & ~0x7;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
