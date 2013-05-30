#include <sstream>

#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


evb::bu::EventTable::EventTable
(
  BU* bu,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<DiskWriter> diskWriter
) :
bu_(bu),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
blockFIFO_("blockFIFO"),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
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
    while ( doProcessing_ && (
        assembleDataBlockMessages() ||
        buildEvents() ||
        handleCompleteEvents()
      ) ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


bool evb::bu::EventTable::assembleDataBlockMessages()
{
  toolbox::mem::Reference* bufRef = 0;
  
  if ( ! ruProxy_->getData(bufRef) ) return false;
  
  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
    (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  
  Index index;
  index.ruTid = stdMsg->InitiatorAddress;
  index.resourceId = dataBlockMsg->buResourceId;
  
  DataBlockMap::iterator dataBlockPos = dataBlockMap_.lower_bound(index);
  if ( dataBlockPos == dataBlockMap_.end() || (dataBlockMap_.key_comp()(index,dataBlockPos->first)) )
  {
    // new data block
    FragmentChainPtr dataBlock( new FragmentChain(dataBlockMsg->nbBlocks) );
    dataBlockPos = dataBlockMap_.insert(dataBlockPos, DataBlockMap::value_type(index,dataBlock));
  }
  
  dataBlockPos->second->append(dataBlockMsg->blockNb,bufRef);
  
  if ( dataBlockPos->second->isComplete() )
  {
    while ( ! blockFIFO_.enq(dataBlockPos->second->head()) ) { ::usleep(1000); }
    dataBlockMap_.erase(dataBlockPos);
  }
  
  return true;
}


bool evb::bu::EventTable::buildEvents()
{
  toolbox::mem::Reference* head = 0;
  
  if ( ! blockFIFO_.deq(head) ) return false;

  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)head->getDataLocation();
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
    (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const I2O_TID ruTid = stdMsg->InitiatorAddress;
  const uint32_t resourceId = dataBlockMsg->buResourceId;
  
  toolbox::mem::Reference* bufRef = head;
  
  while ( bufRef )
  {
    size_t offset = sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
    
    while ( offset < bufRef->getBuffer()->getSize() )
    { 
      unsigned char* payload = (unsigned char*)bufRef->getDataLocation() + offset;
      const msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;
      const EvBid evbId = superFragmentMsg->evbId;
      
      EventMap::iterator eventPos = eventMap_.lower_bound(evbId);
      if ( eventPos == eventMap_.end() || (eventMap_.key_comp()(evbId,eventPos->first)) )
      {
        // new event
        EventPtr event( new Event(runNumber_, evbId.eventNumber(), resourceId, ruProxy_->getRuTids()) );
        eventPos = eventMap_.insert(eventPos, EventMap::value_type(evbId,event));

        ++eventMonitoring_.nbEventsInBU;
      }
      
      eventPos->second->appendSuperFragment(ruTid,bufRef->duplicate(),
        payload+sizeof(msg::SuperFragment),superFragmentMsg->partSize);
      
      offset += superFragmentMsg->partSize;
    }
    
    bufRef = bufRef->getNextReference();  
  }
  
  head->release(); // The bufRef's holding event fragments are now owned by the events
  
  return false;
}


bool evb::bu::EventTable::handleCompleteEvents()
{
  bool foundCompleteEvents = false;
  
  EventMap::iterator eventPos = eventMap_.begin();
  while ( eventPos != eventMap_.end() )
  {
    if ( eventPos->second->isComplete() )
    {
      --eventMonitoring_.nbEventsInBU;
      
      if ( ! dropEventData_ )
        diskWriter_->writeEvent(eventPos->second);
      else
        ++eventMonitoring_.nbEventsDropped;
      
      eventMap_.erase(eventPos++);
      
      foundCompleteEvents = true;
    }
  }
  return foundCompleteEvents;
}
 

void evb::bu::EventTable::appendConfigurationItems(InfoSpaceItems& params)
{
  dropEventData_ = false;

  tableParams_.add("dropEventData", &dropEventData_);

  params.add(tableParams_);
}


void evb::bu::EventTable::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEvtsUnderConstruction_ = 0;
  nbEventsInBU_ = 0;
  nbEvtsBuilt_ = 0;
  rate_ = 0;
  bandwidth_ = 0;
  eventSize_ = 0;
  eventSizeStdDev_ = 0;
  
  items.add("nbEvtsUnderConstruction", &nbEvtsUnderConstruction_);
  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEvtsBuilt", &nbEvtsBuilt_);
  items.add("rate", &rate_);
  items.add("bandwidth", &bandwidth_);
  items.add("eventSize", &eventSize_);
  items.add("eventSizeStdDev", &eventSizeStdDev_);
}


void evb::bu::EventTable::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
  nbEvtsBuilt_ = eventMonitoring_.perf.logicalCount;
  rate_ = eventMonitoring_.perf.logicalRate();
  bandwidth_ = eventMonitoring_.perf.bandwidth();
  eventSize_ = eventMonitoring_.perf.size();
  eventSizeStdDev_ = eventMonitoring_.perf.sizeStdDev();
}


void evb::bu::EventTable::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  eventMonitoring_.nbEventsInBU = 0;
  eventMonitoring_.nbEventsDropped = 0;
  eventMonitoring_.perf.reset();
}


void evb::bu::EventTable::configure(const uint32_t maxEvtsUnderConstruction)
{
  clear();
  
  blockFIFO_.resize(maxEvtsUnderConstruction);
  
  requestEvents_ = true;
}


void evb::bu::EventTable::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( blockFIFO_.deq(bufRef) ) { bufRef->release(); }

  dataBlockMap_.clear();
  eventMap_.clear();
}


void evb::bu::EventTable::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>Event Table</p>"                                    << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events built</td>"                               << std::endl;
    *out << "<td>" << eventMonitoring_.perf.logicalCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events in BU</td>"                               << std::endl;
    *out << "<td>" << eventMonitoring_.nbEventsInBU << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events dropped</td>"                             << std::endl;
    *out << "<td>" << eventMonitoring_.nbEventsDropped << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->setf(std::ios::fixed);
    out->precision(2);
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << bandwidth_ / 0x100000 << "</td>"              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << rate_ << "</td>"                              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>event size (kB)</td>"                              << std::endl;
    *out << "<td>" << eventSize_ << " +/- " << eventSizeStdDev_ << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  blockFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  tableParams_.printHtml("Configuration", out);

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>maxEvtsUnderConstruction</td>"                     << std::endl;
  *out << "<td>" << stateMachine_->maxEvtsUnderConstruction() << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
