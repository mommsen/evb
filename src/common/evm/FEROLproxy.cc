#include "interface/shared/GlobalEventNumber.h"
#include "evb/evm/EVMinput.h"


bool evb::evm::EVMinput::FEROLproxy::getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
{
  return superFragmentFIFO_.deq(superFragment);

  if ( superFragmentMap_.empty() ) return false;

  boost::upgrade_lock<boost::shared_mutex> upgradeLock(superFragmentMapMutex_);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.begin();
  
  while ( fragmentPos != superFragmentMap_.end() )
  {
    if ( fragmentPos->second->isComplete() )
    {
      superFragment = fragmentPos->second;
      
      boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
      superFragmentMap_.erase(fragmentPos);
      
      return true;
    }
    ++fragmentPos;
  }
  return false;
}


uint32_t evb::evm::EVMinput::FEROLproxy::extractTriggerInformation(const unsigned char* payload) const
{
  using namespace evtn;
  
  //set the evm board sense
  if (! set_evm_board_sense(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "Cannot decode EVM board sense");
  }
  
  //check that we've got the TCS chip
  if (! has_evm_tcs(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "No TCS chip found");
  }
  
  //check that we've got the FDL chip
  if (! has_evm_fdl(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "No FDL chip found");
  }
  
  //check that we got the right bunch crossing
  if ( getfdlbxevt(payload) != 0 )
  {
    std::ostringstream msg;
    msg << "Wrong bunch crossing in event (expect 0): "
      << getfdlbxevt(payload);
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }
  
  //extract lumi section number
  //use offline numbering scheme where LS starts with 1
  return getlbn(payload) + 1;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
