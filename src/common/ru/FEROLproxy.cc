#include "evb/ru/RUinput.h"


bool evb::ru::RUinput::FEROLproxy::getSuperFragmentWithEvBid(const EvBid& evbId, readoutunit::FragmentChainPtr& superFragment)
{
  // boost::upgrade_lock<boost::shared_mutex> upgradeLock(superFragmentMapMutex_);
  
  // SuperFragmentMap::iterator fragmentPos = superFragmentMap_.find(evbId);
  
  // if ( fragmentPos == superFragmentMap_.end() || !fragmentPos->second->isComplete() ) return false;
  
  // superFragment = fragmentPos->second;
  // superFragmentMap_.erase(fragmentPos);
  // return true;

  return false;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
