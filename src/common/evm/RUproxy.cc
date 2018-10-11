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

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <string.h>


evb::evm::RUproxy::RUproxy
(
  EVM* evm,
  std::shared_ptr< readoutunit::StateMachine<EVM> > stateMachine
) :
  evm_(evm),
  stateMachine_(stateMachine),
  msgPool_(evm->getMsgPool()),
  readoutMsgFIFO_(evm,"readoutMsgFIFO"),
  doProcessing_(false),
  draining_(false),
  processingActive_(false),
  tid_(0),
  ruCount_(0)
{
  resetMonitoringCounters();
  startRequestWorkLoop();
}


evb::evm::RUproxy::~RUproxy()
{
  if ( processRequestsWL_->isActive() )
    processRequestsWL_->cancel();
}


void evb::evm::RUproxy::startRequestWorkLoop()
{
  try
  {
    processRequestsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( evm_->getIdentifier("processRequests"), "waiting" );

    if ( ! processRequestsWL_->isActive() )
    {
      processRequestsWL_->activate();

      processRequestsAction_ =
        toolbox::task::bind(this, &evb::evm::RUproxy::processRequests,
                            evm_->getIdentifier("processRequests") );
    }
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'processRequests'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void evb::evm::RUproxy::startProcessing()
{
  resetMonitoringCounters();
  if ( participatingRUs_.empty() ) return;

  doProcessing_ = true;
  draining_ = false;

  processRequestsWL_->submit(processRequestsAction_);
}


void evb::evm::RUproxy::drain()
{
  draining_ = true;
  while ( !readoutMsgFIFO_.empty() || processingActive_ ) ::usleep(1000);
}


void evb::evm::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  draining_ = false;
  while ( processingActive_ ) ::usleep(1000);

  readoutMsgFIFO_.clear();
}


toolbox::mem::Reference* evb::evm::RUproxy::getRequestMsgBuffer(const uint32_t bufSize)
{
  toolbox::mem::Reference* rqstBufRef = 0;
  do
  {
    try
    {
      rqstBufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(msgPool_, bufSize);
      rqstBufRef->setDataSize(bufSize);
    }
    catch(toolbox::mem::exception::Exception)
    {
      rqstBufRef = 0;
      if ( ! doProcessing_ )
        throw exception::HaltRequested();
      ::usleep(100);
    }
  } while ( !rqstBufRef );

  return rqstBufRef;
}


void evb::evm::RUproxy::sendRequest(const readoutunit::FragmentRequestPtr& fragmentRequest)
{
  if ( ! participatingRUs_.empty() )
  {
    readoutMsgFIFO_.enqWait(fragmentRequest);
  }
}


bool evb::evm::RUproxy::processRequests(toolbox::task::WorkLoop* wl)
{
  if ( ! doProcessing_ ) return false;

  processingActive_ = true;

  const uint32_t blockSize = evm_->getConfiguration()->allocateBlockSize;
  toolbox::mem::Reference* rqstBufRef = 0;
  uint32_t msgSize = 0;
  unsigned char* payload = 0;
  uint32_t requestCount = 0;
  const uint64_t maxAllocateTime = evm_->getConfiguration()->maxAllocateTime * 1000 * ruCount_;
  uint64_t timeLimit = getTimeStamp() + maxAllocateTime;

  try
  {
    do
    {
      readoutunit::FragmentRequestPtr fragmentRequest;
      while ( readoutMsgFIFO_.deq(fragmentRequest) )
      {
        const uint16_t nbRequests = fragmentRequest->nbRequests;
        const uint16_t ruCount = fragmentRequest->ruTids.size();
        const size_t eventRequestSize = sizeof(msg::EventRequest) +
          nbRequests * sizeof(EvBid) +
          ((ruCount+1)&~1) * sizeof(I2O_TID); // even number of I2O_TIDs to align header to 64-bits

        if ( msgSize+eventRequestSize > blockSize && rqstBufRef )
        {
          sendMsgToRUs(rqstBufRef,msgSize,requestCount);
        }

        if ( ! rqstBufRef )
        {
          rqstBufRef = getRequestMsgBuffer(blockSize);
          msgSize = sizeof(msg::ReadoutMsg);
          payload = ((unsigned char*)rqstBufRef->getDataLocation()) + msgSize;
          timeLimit = getTimeStamp() + maxAllocateTime;
        }

        msg::EventRequest* eventRequest = (msg::EventRequest*)payload;
        eventRequest->msgSize      = eventRequestSize;
        eventRequest->buTid        = fragmentRequest->buTid;
        eventRequest->priority     = 0; //not used on RU
        eventRequest->buResourceId = fragmentRequest->buResourceId;
        eventRequest->timeStampNS  = fragmentRequest->timeStampNS;
        eventRequest->nbRequests   = nbRequests;
        eventRequest->nbDiscards   = fragmentRequest->nbDiscards;
        eventRequest->nbRUtids     = ruCount;
        payload += sizeof(msg::EventRequest);

        memcpy(payload,&fragmentRequest->evbIds[0],nbRequests*sizeof(EvBid));
        payload += nbRequests*sizeof(EvBid);

        memcpy(payload,&fragmentRequest->ruTids[0],ruCount*sizeof(I2O_TID));
        payload += ((ruCount+1)&~1)*sizeof(I2O_TID);
        msgSize += eventRequestSize;
        ++requestCount;

        {
          boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

          allocateMonitoring_.lastEventNumberToRUs = fragmentRequest->evbIds[nbRequests-1].eventNumber();
          allocateMonitoring_.perf.logicalCount += nbRequests*ruCount_;
        }
      }
      ::usleep(10);
    } while ( getTimeStamp() < timeLimit && !draining_ );

    if ( rqstBufRef )
    {
      // send what we have
      sendMsgToRUs(rqstBufRef,msgSize,requestCount);
    }
  }
  catch(exception::HaltRequested& e)
  {
    if (rqstBufRef) rqstBufRef->release();
    processingActive_ = false;
    return false;
  }
  catch(xcept::Exception& e)
  {
    if (rqstBufRef) rqstBufRef->release();
    processingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    if (rqstBufRef) rqstBufRef->release();
    processingActive_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    if (rqstBufRef) rqstBufRef->release();
    processingActive_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  processingActive_ = false;

  return doProcessing_;
}


void evb::evm::RUproxy::sendMsgToRUs(toolbox::mem::Reference*& rqstBufRef, const uint32_t msgSize, uint32_t& requestCount)
{
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
  readoutMsg->nbRequests   = requestCount;

  rqstBufRef->setDataSize(msgSize);
  requestCount = 0;

  uint32_t retries = 0;
  for (ApplicationDescriptorsAndTids::const_iterator it = participatingRUs_.begin(), itEnd = participatingRUs_.end();
       it != itEnd; ++it)
  {
    toolbox::mem::Reference* bufRef = getRequestMsgBuffer(msgSize);
    void* payload = bufRef->getDataLocation();
    memcpy(payload,rqstBufRef->getDataLocation(),msgSize);
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)payload;
    stdMsg->TargetAddress = it->tid;

    try
    {
      retries += evm_->postMessage(bufRef,it->descriptor);
    }
    catch(exception::I2O& e)
    {
      std::ostringstream msg;
      msg << "Failed to send message to RU ";
      msg << it->tid;
      XCEPT_RETHROW(exception::I2O, msg.str(), e);
    }
  }
  rqstBufRef->release();
  rqstBufRef = 0;

  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

    const uint32_t totalSize = msgSize*ruCount_;
    allocateMonitoring_.perf.sumOfSizes += totalSize;
    allocateMonitoring_.perf.sumOfSquares += totalSize*totalSize;
    allocateMonitoring_.perf.i2oCount += ruCount_;
    allocateMonitoring_.perf.retryCount += retries;
  }
}


void evb::evm::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
  allocateRate_ = 0;
  allocateRetryRate_ = 0;

  items.add("allocateRate", &allocateRate_);
  items.add("allocateRetryRate", &allocateRetryRate_);
}


void evb::evm::RUproxy::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

  const double deltaT = allocateMonitoring_.perf.deltaT();
  allocateMonitoring_.throughput = allocateMonitoring_.perf.throughput(deltaT);
  allocateMonitoring_.assignmentRate = allocateMonitoring_.perf.logicalRate(deltaT);
  allocateMonitoring_.i2oRate = allocateMonitoring_.perf.i2oRate(deltaT);
  allocateMonitoring_.retryRate = allocateMonitoring_.perf.retryRate(deltaT);
  allocateMonitoring_.packingFactor = allocateMonitoring_.perf.packingFactor();
  allocateMonitoring_.perf.reset();

  allocateRate_ = allocateMonitoring_.i2oRate;
  allocateRetryRate_ = allocateMonitoring_.retryRate;
}


void evb::evm::RUproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
  allocateMonitoring_.lastEventNumberToRUs = 0;
  allocateMonitoring_.perf.reset();
}


void evb::evm::RUproxy::configure()
{
  getApplicationDescriptors();
  ruCount_ = participatingRUs_.size();

  readoutMsgFIFO_.clear();
  readoutMsgFIFO_.resize(evm_->getConfiguration()->allocateFIFOCapacity);
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
  ruCount_ = 0;
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
    std::ostringstream msg;
    msg << "Failed to get application descriptor for RU ";
    msg << instance.toString();
    XCEPT_RETHROW(exception::Configuration, msg.str(), e);
  }

  try
  {
    ru.tid = i2o::utils::getAddressMap()->getTid(ru.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream msg;
    msg << "Failed to get I2O TID for RU ";
    msg << instance.toString();
    XCEPT_RETHROW(exception::I2O, msg.str(), e);
  }

  if ( std::find(participatingRUs_.begin(),participatingRUs_.end(),ru) != participatingRUs_.end() )
  {
    std::ostringstream msg;
    msg << "Participating RU instance " << instance.toString();
    msg << " is a duplicate.";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  participatingRUs_.push_back(ru);
  ruTids_.push_back(ru.tid);
}


cgicc::div evb::evm::RUproxy::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("RUproxy"));

  {
    table table;
    table.set("title","Statistics of readout messages sent to the RUs. Normally, the allocate FIFO should be empty.");

    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

    table.add(tr()
              .add(td("last event number to RUs"))
              .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.lastEventNumberToRUs))));
    table.add(tr()
              .add(td("throughput (kB/s)"))
              .add(td(doubleToString(allocateMonitoring_.throughput / 1e3,2))));
    table.add(tr()
              .add(td("assignment rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.assignmentRate))));
    table.add(tr()
              .add(td("I2O rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(allocateMonitoring_.i2oRate))));
    table.add(tr()
              .add(td("I2O retry rate (Hz)"))
              .add(td(doubleToString(allocateMonitoring_.retryRate,2))));
    table.add(tr()
              .add(td("Events assigned/I2O"))
              .add(td(doubleToString(allocateMonitoring_.packingFactor,1))));

    const uint32_t activeRUs = ruTids_.empty() ? 0 : ruTids_.size()-1; //exclude the EVM TID
    table.add(tr()
              .add(td("# active RUs"))
              .add(td(boost::lexical_cast<std::string>(activeRUs))));

    div.add(table);
  }

  div.add(readoutMsgFIFO_.getHtmlSnipped());

  return div;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
