#include <sstream>
#include <string>

#include <boost/lexical_cast.hpp>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventBuilder.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


evb::bu::EventBuilder::EventBuilder
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
configuration_(bu->getConfiguration()),
doProcessing_(false),
processActive_(false)
{
  builderAction_ =
    toolbox::task::bind(this, &evb::bu::EventBuilder::process,
      bu_->getIdentifier("eventBuilder") );
}


void evb::bu::EventBuilder::addSuperFragment
(
  const uint32_t buResourceId,
  FragmentChainPtr& superFragments
)
{
  const uint16_t builderId = buResourceId % configuration_->numberOfBuilders;
  while ( ! superFragmentFIFOs_[builderId]->enq(superFragments) ) ::usleep(1000);
}


void evb::bu::EventBuilder::configure()
{
  clear();

  superFragmentFIFOs_.clear();

  for (uint16_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    std::ostringstream fifoName;
    fifoName << "superFragmentFIFO_" << i;
    SuperFragmentFIFOPtr superFragmentFIFO( new SuperFragmentFIFO(fifoName.str()) );
    superFragmentFIFO->resize(configuration_->superFragmentFIFOCapacity);
    superFragmentFIFOs_.insert( SuperFragmentFIFOs::value_type(i,superFragmentFIFO) );
  }

  createProcessingWorkLoops();
}


void evb::bu::EventBuilder::createProcessingWorkLoops()
{
  const std::string identifier = bu_->getIdentifier();

  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=builderWorkLoops_.size(); i < configuration_->numberOfBuilders; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "Builder_" << i;
      toolbox::task::WorkLoop* wl = toolbox::task::getWorkLoopFactory()->getWorkLoop( workLoopName.str(), "waiting" );

      if ( ! wl->isActive() ) wl->activate();
      builderWorkLoops_.push_back(wl);
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void evb::bu::EventBuilder::clear()
{
  FragmentChainPtr superFragments;
  for (SuperFragmentFIFOs::iterator it = superFragmentFIFOs_.begin(), itEnd = superFragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    it->second->clear();
  }
}


void evb::bu::EventBuilder::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  doProcessing_ = true;
  for (uint32_t i=0; i < configuration_->numberOfBuilders; ++i)
  {
    builderWorkLoops_.at(i)->submit(builderAction_);
  }
}


void evb::bu::EventBuilder::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


bool evb::bu::EventBuilder::process(toolbox::task::WorkLoop* wl)
{
  processActive_ = true;

  const std::string wlName =  wl->getName();
  const size_t startPos = wlName.find_last_of("_") + 1;
  const size_t endPos = wlName.find("/",startPos);
  const uint16_t builderId = boost::lexical_cast<uint16_t>( wlName.substr(startPos,endPos-startPos) );

  FragmentChainPtr superFragments;
  SuperFragmentFIFOPtr superFragmentFIFO = superFragmentFIFOs_[builderId];

  EventMapPtr eventMap(new EventMap);

  try
  {
    while ( doProcessing_ )
    {
      if ( superFragmentFIFO->deq(superFragments) )
        buildEvent(superFragments,eventMap);
      else
        ::usleep(10);
    }
  }
  catch(xcept::Exception& e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  processActive_ = false;

  return doProcessing_;
}


void evb::bu::EventBuilder::buildEvent
(
  FragmentChainPtr& superFragments,
  EventMapPtr& eventMap
)
{
  toolbox::mem::Reference* bufRef = superFragments->head()->duplicate();
  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
    (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const I2O_TID ruTid = stdMsg->InitiatorAddress;
  const uint16_t nbSuperFragments = dataBlockMsg->nbSuperFragments;
  uint16_t superFragmentCount = 0;
  const uint32_t blockHeaderSize = dataBlockMsg->getHeaderSize();

  EventMap::iterator eventPos = getEventPos(eventMap,dataBlockMsg,superFragmentCount);

  do
  {
    toolbox::mem::Reference* nextRef = bufRef->getNextReference();
    bufRef->setNextReference(0);

    unsigned char* payload = (unsigned char*)bufRef->getDataLocation() + blockHeaderSize;
    uint32_t remainingBufferSize = bufRef->getDataSize() - blockHeaderSize;

    while ( remainingBufferSize > 0 && nbSuperFragments != superFragmentCount )
    {
      const msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;
      payload += sizeof(msg::SuperFragment);
      remainingBufferSize -= sizeof(msg::SuperFragment);

      // std::cout << remainingBufferSize << "\t" << superFragmentCount << std::endl;
      // std::cout << *superFragmentMsg << std::endl;

      if ( eventPos->second->appendSuperFragment(ruTid,bufRef->duplicate(),
          payload,superFragmentMsg->partSize,superFragmentMsg->totalSize) )
      {
        // the super fragment is complete
        ++superFragmentCount;

        const EventPtr& event = eventPos->second;
        if ( event->isComplete() )
        {
          // the event is complete
          event->checkEvent();
          resourceManager_->eventCompleted(event);

          if ( configuration_->dropEventData )
            resourceManager_->discardEvent(event);
          else
            diskWriter_->writeEvent(event);

          eventMap->erase(eventPos++);
        }

        const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
          (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();

        eventPos = getEventPos(eventMap,dataBlockMsg,superFragmentCount);
      }

      payload += superFragmentMsg->partSize;
      remainingBufferSize -= superFragmentMsg->partSize;
    }

    bufRef->release();
    bufRef = nextRef;

  } while ( bufRef );

  if (superFragmentCount != nbSuperFragments)
  {
    std::ostringstream oss;
    oss << "Incomplete I2O_DATA_BLOCK_MESSAGE_FRAME from RU TID " << ruTid;
    oss << ": expected " << nbSuperFragments << " super fragments, but found " << superFragmentCount;
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
}


evb::bu::EventBuilder::EventMap::iterator evb::bu::EventBuilder::getEventPos
(
  EventMapPtr& eventMap,
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg,
  const uint16_t superFragmentCount
)
{
  const EvBid evbId = dataBlockMsg->evbIds[superFragmentCount];

  EventMap::iterator eventPos = eventMap->lower_bound(evbId);
  if ( eventPos == eventMap->end() || (eventMap->key_comp()(evbId,eventPos->first)) )
  {
    // new event
    EventPtr event( new Event(evbId, dataBlockMsg) );
    eventPos = eventMap->insert(eventPos, EventMap::value_type(evbId,event));
  }
  return eventPos;
}


void evb::bu::EventBuilder::printSuperFragmentFIFOs(xgi::Output* out)
{
  const toolbox::net::URN urn = bu_->getApplicationDescriptor()->getURN();

  for (SuperFragmentFIFOs::iterator it = superFragmentFIFOs_.begin(), itEnd = superFragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    it->second->printHtml(out, urn);
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
