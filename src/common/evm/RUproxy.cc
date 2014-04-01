#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/EVM.h"
#include "evb/evm/RUproxy.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


evb::evm::RUproxy::RUproxy
(
  EVM* evm,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
evm_(evm),
fastCtrlMsgPool_(fastCtrlMsgPool),
allocateFIFO_(evm,"allocateFIFO"),
doProcessing_(false),
assignEventsActive_(false),
tid_(0)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::evm::RUproxy::sendRequest(const readoutunit::FragmentRequestPtr& fragmentRequest)
{
  if ( participatingRUs_.empty() ) return;

  allocateFIFO_.enqWait(fragmentRequest);
}


void evb::evm::RUproxy::startProcessing()
{
  resetMonitoringCounters();

  doProcessing_ = true;

  if ( ! participatingRUs_.empty() )
    assignEventsWL_->submit(assignEventsAction_);
}


void evb::evm::RUproxy::drain()
{
  while ( !allocateFIFO_.empty() || assignEventsActive_ ) ::usleep(1000);
}


void evb::evm::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while ( assignEventsActive_ ) ::usleep(1000);
  allocateFIFO_.clear();
}


void evb::evm::RUproxy::startProcessingWorkLoop()
{
  try
  {
    assignEventsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( evm_->getIdentifier("assignEvents"), "waiting" );

    if ( ! assignEventsWL_->isActive() )
    {
      assignEventsAction_ =
        toolbox::task::bind(this, &evb::evm::RUproxy::assignEvents,
          evm_->getIdentifier("ruProxyAssignEvents") );

      assignEventsWL_->activate();
    }
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'assignEvents'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::evm::RUproxy::assignEvents(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  assignEventsActive_ = true;

  try
  {
    readoutunit::FragmentRequestPtr fragmentRequest;
    while ( allocateFIFO_.deq(fragmentRequest) )
    {
      const uint16_t requestsCount = fragmentRequest->nbRequests;
      const uint16_t ruCount = fragmentRequest->ruTids.size();
      const size_t msgSize = sizeof(msg::ReadoutMsg) +
        requestsCount * sizeof(EvBid) +
        (ruCount|0x1) * sizeof(I2O_TID); // odd number of I2O_TIDs to align header to 64-bits

      toolbox::mem::Reference* rqstBufRef =
        toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_, msgSize);
      rqstBufRef->setDataSize(msgSize);

      I2O_MESSAGE_FRAME* stdMsg =
        (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
      I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
      msg::ReadoutMsg* readoutMsg = (msg::ReadoutMsg*)stdMsg;

      stdMsg->VersionOffset    = 0;
      stdMsg->MsgFlags         = 0;
      stdMsg->MessageSize      = msgSize >> 2;
      stdMsg->InitiatorAddress = tid_;
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      readoutMsg->buTid        = fragmentRequest->buTid;
      readoutMsg->buResourceId = fragmentRequest->buResourceId;
      readoutMsg->nbRequests   = requestsCount;
      readoutMsg->nbRUtids     = ruCount;

      uint32_t lastEventNumberToRUs = 0;
      unsigned char* payload = (unsigned char*)&readoutMsg->evbIds[0];
      for (uint32_t i = 0; i < requestsCount; ++i)
      {
        memcpy(payload,&fragmentRequest->evbIds[i],sizeof(EvBid));
        payload += sizeof(EvBid);
        lastEventNumberToRUs = fragmentRequest->evbIds[i].eventNumber();
      }

      for (uint32_t i = 0; i < ruCount; ++i)
      {
        memcpy(payload,&fragmentRequest->ruTids[i],sizeof(I2O_TID));
        payload += sizeof(I2O_TID);
      }

      sendToAllRUs(rqstBufRef, msgSize);

      // Free the requests message (its copies were sent not it)
      rqstBufRef->release();

      boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

      allocateMonitoring_.lastEventNumberToRUs = lastEventNumberToRUs;
      allocateMonitoring_.payload += msgSize*participatingRUs_.size();
      allocateMonitoring_.logicalCount += requestsCount;
      allocateMonitoring_.i2oCount += participatingRUs_.size();
    }
  }
  catch(xcept::Exception& e)
  {
    assignEventsActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    assignEventsActive_ = false;
    XCEPT_DECLARE(exception::I2O,
      sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    assignEventsActive_ = false;
    XCEPT_DECLARE(exception::I2O,
      sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  assignEventsActive_ = false;

  return doProcessing_;
}


void evb::evm::RUproxy::sendToAllRUs
(
  toolbox::mem::Reference* bufRef,
  const size_t bufSize
) const
{
  ////////////////////////////////////////////////////////////
  // Make a copy of the pairs message under construction    //
  // for each RU and send each copy to its corresponding RU //
  ////////////////////////////////////////////////////////////

  ApplicationDescriptorsAndTids::const_iterator it = participatingRUs_.begin();
  const ApplicationDescriptorsAndTids::const_iterator itEnd = participatingRUs_.end();
  for ( ; it != itEnd ; ++it)
  {
    // Create an empty request message
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, bufSize);
    char* copyFrame  = (char*)(copyBufRef->getDataLocation());

    // Copy the message under construction into
    // the newly created empty message
    memcpy(
      copyFrame,
      bufRef->getDataLocation(),
      bufRef->getDataSize()
    );

    // Set the size of the copy
    copyBufRef->setDataSize(bufSize);

    // Set the I2O TID target address
    ((I2O_MESSAGE_FRAME*)copyFrame)->TargetAddress = it->tid;

    // Send the pairs message to the RU
    try
    {
      evm_->getApplicationContext()->
        postFrame(
          copyBufRef,
          evm_->getApplicationDescriptor(),
          it->descriptor //,
          //i2oExceptionHandler_,
          //it->descriptor
        );
    }
    catch(xcept::Exception& e)
    {
      std::ostringstream oss;
      oss << "Failed to send message to RU TID ";
      oss << it->tid;
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
  }
}


void evb::evm::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
}


void evb::evm::RUproxy::updateMonitoringItems()
{
}


void evb::evm::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    allocateMonitoring_.lastEventNumberToRUs = 0;
    allocateMonitoring_.payload = 0;
    allocateMonitoring_.logicalCount = 0;
    allocateMonitoring_.i2oCount = 0;
  }
}


void evb::evm::RUproxy::configure()
{
  allocateFIFO_.clear();
  allocateFIFO_.resize(evm_->getConfiguration()->fragmentRequestFIFOCapacity);

  getApplicationDescriptors();

  resetMonitoringCounters();
}


void evb::evm::RUproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(evm_->getApplicationDescriptor());
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::I2O,
      "Failed to get I2O TID for this application", e);
  }

  // Clear list of participating RUs
  participatingRUs_.clear();
  ruTids_.clear();
  ruTids_.push_back(tid_);

  for ( xdata::Vector<xdata::UnsignedInteger32>::const_iterator it = evm_->getConfiguration()->ruInstances.begin(),
          itEnd = evm_->getConfiguration()->ruInstances.end(); it != itEnd; ++it )
  {
    fillRUInstance(*it);
  }
}


void evb::evm::RUproxy::fillRUInstance(xdata::UnsignedInteger32 instance)
{
  ApplicationDescriptorAndTid ru;

  try
  {
    ru.descriptor =
      evm_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptor("evb::RU", instance);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    oss << "Failed to get application descriptor for RU ";
    oss << instance.toString();
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }

  try
  {
    ru.tid = i2o::utils::getAddressMap()->getTid(ru.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    oss << "Failed to get I2O TID for RU ";
    oss << instance.toString();
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }

  if ( ! participatingRUs_.insert(ru).second )
  {
    std::stringstream oss;
    oss << "Participating RU instance " << instance.toString();
    oss << " is a duplicate.";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  ruTids_.push_back(ru.tid);
}


cgicc::div evb::evm::RUproxy::getHtmlSnipped() const
{
  using namespace cgicc;

  table table;

  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

    table.add(tr()
      .add(td("last event number to RUs"))
      .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.lastEventNumberToRUs))));
    table.add(tr()
      .add(td("payload (kB)"))
      .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.payload / 1000))));
    table.add(tr()
      .add(td("logical count"))
      .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.logicalCount))));
    table.add(tr()
      .add(td("I2O count"))
      .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.i2oCount))));
  }

  table.add(tr()
    .add(td().set("colspan","2")
      .add(allocateFIFO_.getHtmlSnipped())));

  cgicc::div div;
  div.add(p("RUproxy"));
  div.add(table);
  return div;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
