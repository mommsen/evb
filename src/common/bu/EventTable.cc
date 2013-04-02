#include <sstream>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/FUproxy.h"
#include "evb/bu/FuRqstForResource.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


evb::bu::EventTable::EventTable
(
  boost::shared_ptr<BU> bu,
  boost::shared_ptr<DiskWriter> diskWriter,
  boost::shared_ptr<FUproxy> fuProxy
) :
bu_(bu),
diskWriter_(diskWriter),
fuProxy_(fuProxy),
completeEventsFIFO_("completeEventsFIFO"),
discardFIFO_("discardFIFO"),
freeResourceIdFIFO_("freeResourceIdFIFO"),
doProcessing_(false),
processActive_(false),
requestEvents_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::bu::EventTable::startConstruction
(
  const uint32_t ruCount,
  toolbox::mem::Reference* bufRef
)
{
  boost::mutex::scoped_lock sl(dataMutex_);
  
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  if (block->superFragmentNb != 0)
  {
    std::ostringstream oss;
    
    oss << "Event data block is not that of the trigger data."; 
    oss << " eventNumber: " << block->eventNumber;
    oss << " resyncCount: " << block->resyncCount;
    oss << " buResourceId: " << block->buResourceId;
    oss << " superFragmentNb: " << block->superFragmentNb;
   
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  // Check that the slot in the event table is free
  Data::iterator pos = data_.lower_bound(block->buResourceId);
  if ( pos != data_.end() && !(data_.key_comp()(block->buResourceId,pos->first)) )
  {
    std::ostringstream oss;
    
    oss << "A super-fragment is already in the lookup table.";
    oss << " eventNumber: " << block->eventNumber;
    oss << " resyncCount: " << block->resyncCount;
    oss << " buResourceId: " << block->buResourceId;
    oss << " superFragmentNb: " << block->superFragmentNb;
    
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  ++eventMonitoring_.nbEventsUnderConstruction;

  EventPtr event( new Event(ruCount, bufRef) );
  data_.insert(pos, Data::value_type(block->buResourceId,event));

  checkForCompleteEvent(event);
}


void evb::bu::EventTable::appendSuperFragment
(
  toolbox::mem::Reference* bufRef
)
{
  boost::mutex::scoped_lock sl(dataMutex_);
  
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  if (block->superFragmentNb == 0)
  {
    std::ostringstream oss;
    
    oss << "Cannot append a block from the EVM.";
    oss << " eventNumber: " << block->eventNumber;
    oss << " resyncCount: " << block->resyncCount;
    oss << " buResourceId: " << block->buResourceId;
    oss << " superFragmentNb: " << block->superFragmentNb;
   
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  Data::iterator pos = data_.find(block->buResourceId);
  if ( pos == data_.end() )
  {
    std::ostringstream oss;
    
    oss << "Cannot append a super fragment from RU to an event which is not under construction.";
    oss << " eventNumber: " << block->eventNumber;
    oss << " resyncCount: " << block->resyncCount;
    oss << " buResourceId: " << block->buResourceId;
    oss << " superFragmentNb: " << block->superFragmentNb;
   
    XCEPT_RAISE(exception::EventOrder, oss.str());
  } 
  
  pos->second->appendSuperFragment(bufRef);

  checkForCompleteEvent(pos->second);
}


void evb::bu::EventTable::checkForCompleteEvent(EventPtr event)
{
  if ( ! event->isComplete() ) return;
  
  updateEventCounters(event);

  if ( dropEventData_ )
  {
    discardEvent( event->buResourceId() );
  }
  else
  {
    while ( ! completeEventsFIFO_.enq(event) ) ::usleep(1000);
  }
}


void evb::bu::EventTable::updateEventCounters(EventPtr event)
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  --eventMonitoring_.nbEventsUnderConstruction;
  ++eventMonitoring_.nbEventsBuilt;
  ++eventMonitoring_.nbEventsInBU;
  
  const size_t payload = event->payload();
  eventMonitoring_.payload += payload;
  eventMonitoring_.payloadSquared += payload*payload;
  
  if ( dropEventData_ )
    ++eventMonitoring_.nbEventsDropped;
}


void evb::bu::EventTable::discardEvent(const uint32_t buResourceId)
{
  boost::mutex::scoped_lock sl(discardFIFOmutex_);
  while ( ! discardFIFO_.enq(buResourceId) ) ::usleep(1000);
}


void evb::bu::EventTable::startProcessing()
{
  for (uint32_t buResourceId = 0; buResourceId < freeResourceIdFIFO_.size(); ++buResourceId)
    freeResourceIdFIFO_.enq(buResourceId);

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
    while (
      doProcessing_ && (
        sendEvtIdRqsts() ||
        handleDiscards() ||
        handleNextCompleteEvent()
      )
    ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


bool evb::bu::EventTable::handleNextCompleteEvent()
{
  if ( diskWriter_->enabled() )
    return writeNextCompleteEvent();
  else
    return sendNextCompleteEventToFU();

  return false;
}


bool evb::bu::EventTable::writeNextCompleteEvent()
{
  EventPtr event;
  if ( ! completeEventsFIFO_.deq(event) ) return false;
  
  diskWriter_->writeEvent(event);
  
  return true;
}


bool evb::bu::EventTable::sendNextCompleteEventToFU()
{
  FuRqstForResource rqst;
  if ( completeEventsFIFO_.empty() ) return false;
  if ( ! fuProxy_->getNextRequest(rqst) ) return false;
  
  EventPtr event;
  completeEventsFIFO_.deq(event);
  event->sendToFU(fuProxy_, rqst);
  
  return true;
}


bool evb::bu::EventTable::handleDiscards()
{
  uint32_t buResourceId;
  if ( ! discardFIFO_.deq(buResourceId) ) return false;

  // msg::EvtIdRqstAndOrRelease rqstAndOrRelease;
  // freeEventAndGetReleaseMsg(buResourceId, rqstAndOrRelease);
  
  // evmProxy_->sendEvtIdRqstAndOrRelease(rqstAndOrRelease);
  
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  --eventMonitoring_.nbEventsInBU;
  
  return true;
}


// void evb::bu::EventTable::freeEventAndGetReleaseMsg
// (
//   const uint32_t buResourceId,
//   msg::EvtIdRqstAndOrRelease& rqstAndOrRelease
// )
// {
//   boost::mutex::scoped_lock sl(dataMutex_);
  
//   Data::iterator pos = data_.find(buResourceId);
//   if ( pos == data_.end() )
//   {
//     std::ostringstream oss;
    
//     oss << "Cannot discard a non-existing event with the BU resource id "
//       << buResourceId;
   
//     XCEPT_RAISE(exception::EventOrder, oss.str());
//   }
  
//   // Prepare a resource request to release an event id
//   rqstAndOrRelease.requestType = msg::EvtIdRqstAndOrRelease::RELEASE;
//   rqstAndOrRelease.evbId       = pos->second->evbId();
//   rqstAndOrRelease.resourceId  = buResourceId;
  
//   // Request another event if we want one
//   if ( requestEvents_ )
//     rqstAndOrRelease.requestType |= msg::EvtIdRqstAndOrRelease::REQUEST;
//   else
//     while ( ! freeResourceIdFIFO_.enq(buResourceId) ) { ::usleep(1000); }
  
//   // Free the resource
//   data_.erase(pos);
// }


// bool evb::bu::EventTable::sendEvtIdRqsts()
// {
//   msg::EvtIdRqstAndOrRelease rqstAndOrRelease;
//   uint32_t buResourceId;
  
//   if ( requestEvents_ && freeResourceIdFIFO_.deq(buResourceId) )
//   {
//     // Prepare an event id request
//     rqstAndOrRelease.requestType = msg::EvtIdRqstAndOrRelease::REQUEST;
//     rqstAndOrRelease.resourceId  = buResourceId;
    
//     // evmProxy_->sendEvtIdRqstAndOrRelease(rqstAndOrRelease);

//     return true;
//   }
  
//   return false;
// }


void evb::bu::EventTable::appendConfigurationItems(InfoSpaceItems& params)
{
  dropEventData_ = false;

  tableParams_.add("dropEventData", &dropEventData_);

  params.add(tableParams_);
}


void evb::bu::EventTable::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEvtsUnderConstruction_ = 0;
  nbEvtsReady_ = 0;
  nbEventsInBU_ = 0;
  nbEvtsBuilt_ = 0;
  
  items.add("nbEvtsUnderConstruction", &nbEvtsUnderConstruction_);
  items.add("nbEvtsReady", &nbEvtsReady_);
  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEvtsBuilt", &nbEvtsBuilt_);
}


void evb::bu::EventTable::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  nbEvtsUnderConstruction_ =  eventMonitoring_.nbEventsUnderConstruction;
  nbEvtsBuilt_ = eventMonitoring_.nbEventsBuilt;
  nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
  
  nbEvtsReady_ = completeEventsFIFO_.elements();
}


void evb::bu::EventTable::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  eventMonitoring_.nbEventsUnderConstruction = 0;
  eventMonitoring_.nbEventsBuilt = 0;
  eventMonitoring_.nbEventsInBU = 0;
  eventMonitoring_.nbEventsDropped = 0;
  eventMonitoring_.payload = 0;
  eventMonitoring_.payloadSquared = 0;
}


void evb::bu::EventTable::getPerformance(PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  performanceMonitor.N = eventMonitoring_.nbEventsBuilt;
  performanceMonitor.sumOfSizes = eventMonitoring_.payload;
  performanceMonitor.sumOfSquares = eventMonitoring_.payloadSquared;
}


void evb::bu::EventTable::configure(const uint32_t maxEvtsUnderConstruction)
{
  clear();
  
  completeEventsFIFO_.resize(maxEvtsUnderConstruction);
  discardFIFO_.resize(maxEvtsUnderConstruction);
  freeResourceIdFIFO_.resize(maxEvtsUnderConstruction);

  requestEvents_ = true;
}


void evb::bu::EventTable::clear()
{
  boost::mutex::scoped_lock sl(dataMutex_);
  
  EventPtr event;
  while ( completeEventsFIFO_.deq(event) ) { event.reset(); }

  uint32_t buResourceId;
  while ( discardFIFO_.deq(buResourceId) ) {};
  while ( freeResourceIdFIFO_.deq(buResourceId) ) {};

  data_.clear();
}


void evb::bu::EventTable::printMonitoringInformation(xgi::Output *out)
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># events built</td>"                               << std::endl;
  *out << "<td>" << eventMonitoring_.nbEventsBuilt << "</td>"     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># events under construction</td>"                  << std::endl;
  *out << "<td>" << eventMonitoring_.nbEventsUnderConstruction << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># complete events in BU</td>"                      << std::endl;
  *out << "<td>" << eventMonitoring_.nbEventsInBU << "</td>"      << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># events dropped</td>"                             << std::endl;
  *out << "<td>" << eventMonitoring_.nbEventsDropped << "</td>"   << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


void evb::bu::EventTable::printQueueInformation(xgi::Output *out)
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  completeEventsFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  discardFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  freeResourceIdFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
