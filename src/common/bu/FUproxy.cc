#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/FUproxy.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"


evb::bu::FUproxy::FUproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
requestFIFO_("requestFIFO")
{
  resetMonitoringCounters();
}


void evb::bu::FUproxy::allocateI2Ocallback(toolbox::mem::Reference* bufRef)
{
  const I2O_BU_ALLOCATE_MESSAGE_FRAME* msg =
    (I2O_BU_ALLOCATE_MESSAGE_FRAME*)bufRef->getDataLocation();
  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)msg;
  const I2O_TID fuTid = stdMsg->InitiatorAddress;

  updateAllocateCounters(msg->n);
  updateParticipatingFUs(fuTid);

  pushRequestOntoFIFO(msg, fuTid);

  bufRef->release();
}


void evb::bu::FUproxy::updateAllocateCounters(const uint32_t nbElements)
{
  boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

  allocateMonitoring_.payload += nbElements * sizeof(BU_ALLOCATE);
  allocateMonitoring_.logicalCount += nbElements;
  ++allocateMonitoring_.i2oCount;
}


void evb::bu::FUproxy::updateParticipatingFUs(const I2O_TID& fuTid)
{
  const std::string errorMsg =
    "Failed to add FU information to participatingFUs";
  try
  {
    // Insert FU information into set (assures uniqueness)
    boost::mutex::scoped_lock sl(participatingFUsMutex_);
    participatingFUs_.insert(fuTid);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::I2O,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + "unknown exception");
  }
}


void evb::bu::FUproxy::pushRequestOntoFIFO
(
  const I2O_BU_ALLOCATE_MESSAGE_FRAME* msg,
  const I2O_TID& fuTid
)
{
  FuRqstForResource rqstForResource;
  rqstForResource.fuTid = fuTid;
  
  for (uint32_t i=0; i<msg->n; ++i)
  {
    rqstForResource.fuTransactionId = msg->allocate[i].fuTransactionId;
    
    while ( ! requestFIFO_.enq(rqstForResource) ) ::usleep(1000);
  }
}


void evb::bu::FUproxy::collectI2Ocallback(toolbox::mem::Reference* bufRef)
{
  bufRef->release();
  
  LOG4CPLUS_ERROR(logger_,
    "I2O_BU_COLLECT is not supported in this version");
}


void evb::bu::FUproxy::discardI2Ocallback(toolbox::mem::Reference* bufRef)
{
  const I2O_BU_DISCARD_MESSAGE_FRAME* msg =
    (I2O_BU_DISCARD_MESSAGE_FRAME*)bufRef->getDataLocation();

  updateDiscardCounters(msg->n);

  for (uint32_t i=0; i<msg->n; ++i)
  {
    eventTable_->discardEvent(msg->buResourceId[i]);
  }

  bufRef->release();
}


void evb::bu::FUproxy::updateDiscardCounters(const uint32_t nbElements)
{
  boost::mutex::scoped_lock sl(discardMonitoringMutex_);

  discardMonitoring_.payload += nbElements * sizeof(U32);
  discardMonitoring_.logicalCount += nbElements;
  ++discardMonitoring_.i2oCount;
}


void evb::bu::FUproxy::handleEoLSmsg(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg)
{
  ((I2O_MESSAGE_FRAME*)msg)->InitiatorAddress = tid_;
  broadcastEoLSmsg(msg);
}


bool evb::bu::FUproxy::getNextRequest(FuRqstForResource& rqst)
{
  return requestFIFO_.deq(rqst);
}


void evb::bu::FUproxy::sendSuperFragment
(
  const FuRqstForResource& rqst,
  const uint32_t superFragmentNb,
  const uint32_t nbSuperFragmentsInEvent,
  toolbox::mem::Reference* head
)
{
  toolbox::mem::Reference* bufRef = head;
  uint32_t eventNumber = 0;
  uint32_t payload = 0;
  uint32_t i2oCount = 0;

  // Prepare each event data block for the FU
  while (bufRef)
  {
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    stdMsg->InitiatorAddress = tid_;
    stdMsg->TargetAddress    = rqst.fuTid;
    
    I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    pvtMsg->XFunctionCode = I2O_FU_TAKE;
    
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
    block->fuTransactionId         = rqst.fuTransactionId;
    block->nbSuperFragmentsInEvent = nbSuperFragmentsInEvent;
    block->superFragmentNb         = superFragmentNb;
    
    eventNumber = block->eventNumber;
    payload += (stdMsg->MessageSize << 2) -
      sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
    ++i2oCount;
    
    bufRef = bufRef->getNextReference();
  }
  
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    
    dataMonitoring_.i2oCount += i2oCount;
    dataMonitoring_.payload += payload;
    if ( superFragmentNb == nbSuperFragmentsInEvent-1 )
    {
      dataMonitoring_.lastEventNumberToFUs = eventNumber;
      ++dataMonitoring_.logicalCount;
    }
  }

  xdaq::ApplicationDescriptor *fu = 0;
  try
  {
    fu = i2o::utils::getAddressMap()->getApplicationDescriptor(rqst.fuTid);
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream oss;
    
    oss << "Failed to get application descriptor for FU with tid ";
    oss << rqst.fuTid;
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  
  try
  {
    app_->getApplicationContext()->
      postFrame(
        head,
        app_->getApplicationDescriptor(),
        fu//,
        //i2oExceptionHandler_,
        //fu
      );
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream oss;
    
    oss << "Failed to send super fragment to FU ";
    oss << fu->getInstance();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }
}


void evb::bu::FUproxy::appendConfigurationItems(InfoSpaceItems& params)
{
  requestFIFOCapacity_ = 512;

  fuParams_.add("requestFIFOCapacity", &requestFIFOCapacity_);

  params.add(fuParams_);
}


void evb::bu::FUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
  lastEventNumberToFUs_ = 0;
  i2oBUAllocCount_ = 0;
  i2oFUTakeCount_ = 0;
  i2oBUDiscardCount_ = 0;

  items.add("lastEventNumberToFUs", &lastEventNumberToFUs_);
  items.add("i2oBUAllocCount", &i2oBUAllocCount_);
  items.add("i2oBUDiscardCount", &i2oBUDiscardCount_);
  items.add("i2oFUTakeCount", &i2oFUTakeCount_);
}


void evb::bu::FUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    i2oBUAllocCount_ = allocateMonitoring_.logicalCount;
  }

  {
    boost::mutex::scoped_lock sl(discardMonitoringMutex_);
    i2oBUDiscardCount_ = discardMonitoring_.logicalCount;
  }

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    i2oFUTakeCount_ = dataMonitoring_.logicalCount;
    lastEventNumberToFUs_ = dataMonitoring_.lastEventNumberToFUs;
  }
  
}


void evb::bu::FUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    allocateMonitoring_.payload = 0;
    allocateMonitoring_.logicalCount = 0;
    allocateMonitoring_.i2oCount = 0;
  }
  
  {
    boost::mutex::scoped_lock sl(discardMonitoringMutex_);
    discardMonitoring_.payload = 0;
    discardMonitoring_.logicalCount = 0;
    discardMonitoring_.i2oCount = 0;
  }
  
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    dataMonitoring_.payload = 0;
    dataMonitoring_.logicalCount = 0;
    dataMonitoring_.i2oCount = 0;
    dataMonitoring_.lastEventNumberToFUs = 0;
  }
  
  {
    boost::mutex::scoped_lock sl(eolsMonitoringMutex_);
    eolsMonitoring_.payload = 0;
    eolsMonitoring_.logicalCount = 0;
    eolsMonitoring_.i2oCount = 0;
    eolsMonitoring_.lastLumiSection = 0;
  }
}


void evb::bu::FUproxy::configure()
{
  clear();

  requestFIFO_.resize(requestFIFOCapacity_);
}


void evb::bu::FUproxy::clear()
{
  FuRqstForResource rqst;
  while( requestFIFO_.deq(rqst) ) {}
  
  boost::mutex::scoped_lock sl(participatingFUsMutex_);
  participatingFUs_.clear();
}


void evb::bu::FUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>FUproxy</p>"                                        << std::endl;

  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number to FUs</td>"                       << std::endl;
    *out << "<td>" << dataMonitoring_.lastEventNumberToFUs << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Event data</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << dataMonitoring_.payload / 0x100000 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << dataMonitoring_.logicalCount << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << dataMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Allocate request</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << allocateMonitoring_.payload / 0x400 << "</td>"<< std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << allocateMonitoring_.logicalCount << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << allocateMonitoring_.i2oCount << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(discardMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Discards</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << discardMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << discardMonitoring_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << discardMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(eolsMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">EoLS msg</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last EoLS signal</td>"                             << std::endl;
    *out << "<td>" << eolsMonitoring_.lastLumiSection << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (bytes)</td>"                              << std::endl;
    *out << "<td>" << eolsMonitoring_.payload << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << eolsMonitoring_.logicalCount << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << eolsMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  requestFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  fuParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void evb::bu::FUproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(app_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }


  const std::string errorMsg =
    "Failed to clear internal descriptor list of participating FUs: ";

  try
  {
    boost::mutex::scoped_lock sl(participatingFUsMutex_);
    participatingFUs_.clear();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::Configuration,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::Configuration,
      errorMsg + "unknown exception");
  }
}


void evb::bu::FUproxy::broadcastEoLSmsg(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg)
{
  if ( participatingFUs_.empty() ) return;

  const std::string errorMsg =
    "Failed to send end-of-lumi-section signal to FUs";
  
  try
  {
    boost::mutex::scoped_lock sl(participatingFUsMutex_);
    
    /////////////////////////////////////////////////////////////////////////
    // Make a copy of the end-of-lumisection message under construction    //
    // for each FU and send each copy to its corresponding FU              //
    /////////////////////////////////////////////////////////////////////////
    
    for (ParticipatingFUs::const_iterator it=participatingFUs_.begin(),
           itEnd=participatingFUs_.end();
         it != itEnd; ++it)
    {
      // Create an empty message
      toolbox::mem::Reference* copyBufRef =
        toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_,
          sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
      char* copyFrame  = (char*)(copyBufRef->getDataLocation());
            
      // Copy the message under construction into
      // the newly created empty message
      memcpy(
        copyFrame,
        msg,
        sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME)
      );
      
      // Set the size of the copy
      copyBufRef->setDataSize(sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
      
      // Set the I2O TID target address
      ((I2O_MESSAGE_FRAME*)copyFrame)->TargetAddress = *it;
      xdaq::ApplicationDescriptor *fu =
        i2o::utils::getAddressMap()->getApplicationDescriptor(*it);

      // Send the pairs message to the FU
      try
      {
        app_->getApplicationContext()->
          postFrame(
            copyBufRef,
            app_->getApplicationDescriptor(),
            fu//,
            //i2oExceptionHandler_,
            //fu
          );
      }
      catch(xcept::Exception &e)
      {
        std::ostringstream oss;
        
        oss << "Failed to send end-of-lumisection message to FU";
        oss << fu->getInstance();
        oss << ": ";
        oss << xcept::stdformat_exception_history(e);
        
        LOG4CPLUS_WARN(logger_, oss.str());
      }
    }
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::I2O,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + "unknown exception");
  }
  
  size_t msgCount = participatingFUs_.size();

  {
    boost::mutex::scoped_lock sl(eolsMonitoringMutex_);
    eolsMonitoring_.payload += msgCount * sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME);
    eolsMonitoring_.logicalCount++;
    eolsMonitoring_.i2oCount += msgCount;
    eolsMonitoring_.lastLumiSection = msg->lumiSection;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
