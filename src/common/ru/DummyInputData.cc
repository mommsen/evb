#include "evb/ru/InputHandler.h"
#include "evb/Exception.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"


evb::ru::DummyInputData::DummyInputData() :
eventNumber_(0)
{}


void evb::ru::DummyInputData::configure(const Configuration& conf)
{
  clear();
  
  toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
  try
  {
    toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    // don't care
  }
  
  try
  {
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(conf.fragmentPoolSize);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for dummy fragments.", e);
  }
  
  fragmentTrackers_.clear();
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
    fragmentTrackers_.insert(FragmentTrackers::value_type(fedId,
        FragmentTracker(fedId,conf.dummyFedSize,conf.dummyFedSizeStdDev)));
  }
}


void evb::ru::DummyInputData::dataReadyCallback(toolbox::mem::Reference* bufRef)
{
  XCEPT_RAISE(exception::Configuration,
    "Received an event fragment while generating dummy data");
}


bool evb::ru::DummyInputData::getNextAvailableSuperFragment(SuperFragmentPtr& superFragment)
{
  if (++eventNumber_ % (1 << 24) == 0) eventNumber_ = 1;
  const EvBid evbId = evbIdFactory_.getEvBid(eventNumber_);

  return getSuperFragmentWithEvBid(evbId,superFragment);
}


bool evb::ru::DummyInputData::getSuperFragmentWithEvBid(const EvBid& evbId, SuperFragmentPtr& superFragment)
{
  superFragment.reset( new SuperFragment(evbId, fedList_) );
  
  toolbox::mem::Reference* bufRef = 0;
  const uint32_t ferolBlockSize = 4*1024;
  
  for ( FragmentTrackers::iterator it = fragmentTrackers_.begin(), itEnd = fragmentTrackers_.end();
        it != itEnd; ++it)
  {
    uint32_t remainingFedSize = it->second.startFragment(evbId.eventNumber());
    uint32_t packetNumber = 0;
    
    try
    {
      bufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fragmentPool_,remainingFedSize+sizeof(ferolh_t));
    }
    catch(xcept::Exception& e)
    {
      return false;
    }
    
    unsigned char* frame = (unsigned char*)bufRef->getDataLocation();
    bzero(bufRef->getDataLocation(), bufRef->getBuffer()->getSize());
    
    while ( remainingFedSize > 0 )
    {
      assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
      uint32_t length;
      
      ferolh_t* ferolHeader = (ferolh_t*)frame;
      ferolHeader->set_signature();
      ferolHeader->set_packet_number(packetNumber);
      
      if (packetNumber == 0)
        ferolHeader->set_first_packet();
      
      if ( remainingFedSize > ferolBlockSize )
      {
        length = ferolBlockSize;
      }
      else
      {
        length = remainingFedSize;
        ferolHeader->set_last_packet();
      }
      remainingFedSize -= length;
      frame += sizeof(ferolh_t);
      
      const size_t filledBytes = it->second.fillData(frame, length);
      
      ferolHeader->set_data_length(filledBytes);
      ferolHeader->set_fed_id(it->first);
      ferolHeader->set_event_number(eventNumber_);
      
      frame += filledBytes;
      
      ++packetNumber;
      assert(packetNumber < 2048);
    }
    
    superFragment->append(it->first,bufRef);
  }

  return true;
}


void evb::ru::DummyInputData::clear()
{
  eventNumber_ = 0;
  evbIdFactory_.reset();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
