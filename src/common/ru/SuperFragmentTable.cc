#include <algorithm>
#include <byteswap.h>
#include <sstream>

#include "interface/shared/i2ogevb2g.h"
#include "evb/RU.h"
#include "evb/ru/SuperFragmentTable.h"
#include "evb/Exception.h"
#include "xcept/tools.h"


evb::ru::SuperFragmentTable::SuperFragmentTable
(
  boost::shared_ptr<RU> ru
) :
ru_(ru),
nbSuperFragmentsReadyLocal_(0)
{}


void evb::ru::SuperFragmentTable::addFragment(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  char* i2oPayloadPtr = (char*)bufRef->getDataLocation() +
    sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint64_t h0 = bswap_64(*((uint64_t*)(i2oPayloadPtr)));
  const uint64_t h1 = bswap_64(*((uint64_t*)(i2oPayloadPtr + 8)));	                       
  
  //uint16_t signature = (h0 & 0xFFFF000000000000) >> 48;
  const uint64_t fedId = (h1 & 0x0FFF000000000000) >> 48;
  const uint64_t eventNumber = (h0 & 0x0000000000FFFFFF);
  
  const EvBid evbId = evbIdFactory_.getEvBid(eventNumber);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.lower_bound(evbId);
  if ( fragmentPos == superFragmentMap_.end() || (superFragmentMap_.key_comp()(evbId,fragmentPos->first)) )
  {
    // new super-fragment
    SuperFragmentPtr superFragment( new SuperFragment(evbId,fedList_) );
    superFragmentMap_.insert(fragmentPos, SuperFragmentMap::value_type(evbId,superFragment));
  }
  const SuperFragmentPtr superFragment = fragmentPos->second;
  
  if ( ! superFragment->append(fedId,bufRef) )
  {
    // error condition
    std::ostringstream msg;
    if ( std::find(fedList_.begin(),fedList_.end(),fedId) == fedList_.end() )
    {
      msg << "The received FED id " << fedId;
      msg << " is not in the excepted list ";
      std::copy(fedList_.begin(), fedList_.end(),
        std::ostream_iterator<uint16_t>(msg,","));
    }
    else
    {
      msg << "Received a duplicated FED id " << fedId;
    }
    msg << " for event " << eventNumber;
    
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }
  
  if ( superFragment->isComplete() )
  {
    if ( dropInputData_ )
      superFragment->head()->release();
    else
      ++nbSuperFragmentsReadyLocal_;
  }
}


bool evb::ru::SuperFragmentTable::getNextAvailableSuperFragment(SuperFragmentPtr superFragment)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.begin();
  
  while ( fragmentPos != superFragmentMap_.end() )
  {
    if ( fragmentPos->second->isComplete() )
    {
      superFragment = fragmentPos->second;
      superFragmentMap_.erase(fragmentPos);
      --nbSuperFragmentsReadyLocal_;
      return true;
    }
    ++fragmentPos;
  }
  return false;
}


bool evb::ru::SuperFragmentTable::getSuperFragmentWithEvBid(const EvBid& evbId, SuperFragmentPtr superFragment)
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.find(evbId);
  
  if ( fragmentPos == superFragmentMap_.end() || !fragmentPos->second->isComplete() ) return false;
  
  superFragment = fragmentPos->second;
  superFragmentMap_.erase(fragmentPos);
  --nbSuperFragmentsReadyLocal_;
  return true;
}


void evb::ru::SuperFragmentTable::configure()
{
}


void evb::ru::SuperFragmentTable::appendConfigurationItems(InfoSpaceItems& params)
{
  dropInputData_ = false;
  
  // Default is 8 FEDs per super-fragment
  const uint32_t instance = ru_->getApplicationDescriptor()->getInstance();
  const uint32_t firstSourceId = (instance * 8) + 1;
  const uint32_t lastSourceId  = (instance * 8) + 8;

  for (uint32_t sourceId=firstSourceId; sourceId<=lastSourceId; ++sourceId)
  {
    fedSourceIds_.push_back(sourceId);
  }
  
  superFragmentTableParams_.clear();
  superFragmentTableParams_.add("dropInputData", &dropInputData_);
  superFragmentTableParams_.add("fedSourceIds", &fedSourceIds_);

  params.add(superFragmentTableParams_);
}


void evb::ru::SuperFragmentTable::appendMonitoringItems(InfoSpaceItems& items)
{
  nbSuperFragmentsReady_ = 0;
  
  items.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_);
}


void evb::ru::SuperFragmentTable::updateMonitoringItems()
{
  nbSuperFragmentsReady_ = nbSuperFragmentsReadyLocal_;
}


void evb::ru::SuperFragmentTable::resetMonitoringCounters()
{
  nbSuperFragmentsReady_ = 0;
}


void evb::ru::SuperFragmentTable::clear()
{
  boost::mutex::scoped_lock sl(superFragmentMapMutex_);
  superFragmentMap_.clear();
  
  nbSuperFragmentsReadyLocal_ = 0;
  evbIdFactory_.reset();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
