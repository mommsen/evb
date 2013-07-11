#include <sstream>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


evb::bu::EventTable::EventTable
(
  BU* bu,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<DiskWriter> diskWriter,
  boost::shared_ptr<ResourceManager> resourceManager
) :
bu_(bu),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
resourceManager_(resourceManager),
doProcessing_(false),
processActive_(false)
{
  startProcessingWorkLoop();
}


void evb::bu::EventTable::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void evb::bu::EventTable::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


void evb::bu::EventTable::startProcessingWorkLoop()
{
  try
  {
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("EventTableProcessing"), "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &evb::bu::EventTable::process,
          bu_->getIdentifier("eventTableProcess") );
    
      processingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'EventTableProcessing'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::EventTable::process(toolbox::task::WorkLoop*)
{
  ::usleep(1000);
  
  processActive_ = true;
  
  try
  {
    while ( doProcessing_ && buildEvents() ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


bool evb::bu::EventTable::buildEvents()
{
  FragmentChainPtr superFragments;
  
  if ( ! ruProxy_->getData(superFragments) ) return false;

  toolbox::mem::Reference* bufRef = superFragments->head();
  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
    (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const I2O_TID ruTid = stdMsg->InitiatorAddress;
  const uint16_t nbSuperFragments = dataBlockMsg->nbSuperFragments;
  uint16_t superFragmentCount = 0;
  const uint32_t blockHeaderSize = dataBlockMsg->getHeaderSize();

  EventMap::iterator eventPos = getEventPos(dataBlockMsg,superFragmentCount);

  do
  {
    toolbox::mem::Reference* nextRef = bufRef->getNextReference();
    bufRef->setNextReference(0);
    
    const unsigned char* payload = (unsigned char*)bufRef->getDataLocation() + blockHeaderSize;
    uint32_t remainingBufferSize = bufRef->getDataSize() - blockHeaderSize;
    
    while ( remainingBufferSize > 0 && nbSuperFragments != superFragmentCount )
    {
      const msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;
      payload += sizeof(msg::SuperFragment);
      remainingBufferSize -= sizeof(msg::SuperFragment);
      
      if ( eventPos->second->appendSuperFragment(ruTid,bufRef->duplicate(),
          payload,superFragmentMsg->partSize,superFragmentMsg->totalSize) )
      {
        // the super fragment is complete
        ++superFragmentCount;
        
        if ( eventPos->second->isComplete() )
        {
          resourceManager_->eventCompleted(eventPos->second);
          diskWriter_->writeEvent(eventPos->second);
          eventMap_.erase(eventPos++);
        }
        
        const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
          (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
        
        eventPos = getEventPos(dataBlockMsg,superFragmentCount);
      }
      
      payload += superFragmentMsg->partSize;
      remainingBufferSize -= superFragmentMsg->partSize;
      
      // std::cout << remainingBufferSize << "\t" << superFragmentCount << std::endl;
      // std::cout << *superFragmentMsg << std::endl;
    }
    
    bufRef = nextRef;
    
  } while ( bufRef );

  if (superFragmentCount != nbSuperFragments)
  {
    std::ostringstream oss;
    oss << "Incomplete I2O_DATA_BLOCK_MESSAGE_FRAME from RU TID " << ruTid;
    oss << ": expected " << nbSuperFragments << " super fragments, but found " << superFragmentCount;
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  return true;
}


evb::bu::EventTable::EventMap::iterator evb::bu::EventTable::getEventPos
(
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg,
  const uint16_t superFragmentCount
)
{
  const EvBid evbId = dataBlockMsg->evbIds[superFragmentCount];
  
  EventMap::iterator eventPos = eventMap_.lower_bound(evbId);
  if ( eventPos == eventMap_.end() || (eventMap_.key_comp()(evbId,eventPos->first)) )
  {
    // new event
    EventPtr event( new Event(evbId, dataBlockMsg) );
    eventPos = eventMap_.insert(eventPos, EventMap::value_type(evbId,event));
  }
  return eventPos;
}


void evb::bu::EventTable::clear()
{
  eventMap_.clear();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
