#include <algorithm>
#include <byteswap.h>
#include <iterator>
#include <string.h>

#include "interface/shared/fed_header.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/i2ogevb2g.h"
#include "evb/ru/InputHandler.h"
#include "evb/Constants.h"
#include "evb/Exception.h"
#include "toolbox/net/URN.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"


evb::ru::FEROLproxy::FEROLproxy() :
dropInputData_(false),
nbSuperFragmentsReady_(0)
{}


void evb::ru::FEROLproxy::configure(const Configuration& conf)
{
  clear();
  
  dropInputData_ = conf.dropInputData;
  triggerFedId_ = conf.triggerFedId;
  
  evbIdFactories_.clear();
  fedList_.clear();
  fedList_.reserve(conf.fedSourceIds.size());
  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = conf.fedSourceIds.begin(), itEnd = conf.fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint16_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      
      oss << "fedSourceId is too large.";
      oss << "Actual value: " << fedId;
      oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    fedList_.push_back(fedId);
    evbIdFactories_.insert(EvBidFactories::value_type(fedId,EvBidFactory()));
  }
  std::sort(fedList_.begin(),fedList_.end());
}


void evb::ru::FEROLproxy::dataReadyCallback(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  unsigned char* payload = (unsigned char*)frame + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  ferolh_t* ferolHeader = (ferolh_t*)payload;
  assert( ferolHeader->signature() == FEROL_SIGNATURE );
  const uint16_t fedId = ferolHeader->fed_id();
  const uint32_t eventNumber = ferolHeader->event_number();
  
  uint32_t lsNumber = 0;
  if ( fedId == triggerFedId_ )
  {
    lsNumber = extractTriggerInformation(payload);
  }

  const EvBid evbId = evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
  //std::cout << "**** got EvBid " << evbId << " from FED " << fedId << std::endl;
  //bufRef->release(); return;
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.lower_bound(evbId);
  if ( fragmentPos == superFragmentMap_.end() || (superFragmentMap_.key_comp()(evbId,fragmentPos->first)) )
  {
    // new super-fragment
    FragmentChainPtr superFragment( new FragmentChain(evbId,fedList_) );
    fragmentPos = superFragmentMap_.insert(fragmentPos, SuperFragmentMap::value_type(evbId,superFragment));
  }
  
  if ( ! fragmentPos->second->append(fedId,bufRef) )
  {
    std::ostringstream msg;
    msg << "Received a duplicated FED id " << fedId
      << "for event " << eventNumber;
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }
  
  if ( fragmentPos->second->isComplete() )
  {
    if ( dropInputData_ )
      superFragmentMap_.erase(fragmentPos);
    else
      ++nbSuperFragmentsReady_;
  }
}


bool evb::ru::FEROLproxy::getNextAvailableSuperFragment(FragmentChainPtr& superFragment)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.begin();
  
  while ( fragmentPos != superFragmentMap_.end() )
  {
    if ( fragmentPos->second->isComplete() )
    {
      superFragment = fragmentPos->second;
      superFragmentMap_.erase(fragmentPos);
      --nbSuperFragmentsReady_;
      return true;
    }
    ++fragmentPos;
  }
  return false;
}


bool evb::ru::FEROLproxy::getSuperFragmentWithEvBid(const EvBid& evbId, FragmentChainPtr& superFragment)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.find(evbId);
  
  if ( fragmentPos == superFragmentMap_.end() || !fragmentPos->second->isComplete() ) return false;
  
  superFragment = fragmentPos->second;
  superFragmentMap_.erase(fragmentPos);
  --nbSuperFragmentsReady_;
  return true;
}


void evb::ru::FEROLproxy::startProcessing(const uint32_t runNumber)
{
  for ( EvBidFactories::iterator it = evbIdFactories_.begin(), itEnd = evbIdFactories_.end();
        it != itEnd; ++it)
    it->second.reset(runNumber);
}


void evb::ru::FEROLproxy::clear()
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  superFragmentMap_.clear();
  nbSuperFragmentsReady_ = 0;
}


uint32_t evb::ru::FEROLproxy::extractTriggerInformation(const unsigned char* payload) const
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
