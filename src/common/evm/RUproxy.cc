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
  boost::shared_ptr< readoutunit::StateMachine<EVM> > stateMachine,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
  evm_(evm),
  stateMachine_(stateMachine),
  fastCtrlMsgPool_(fastCtrlMsgPool),
  readoutMsgFIFO_(evm,"readoutMsgFIFO"),
  numberOfAllocators_(0),
  ruCount_(0),
  doProcessing_(false),
  processingActive_(false),
  tid_(0)
{
  resetMonitoringCounters();
  startRequestWorkLoop();

  allocateAction_ =
    toolbox::task::bind(this, &evb::evm::RUproxy::allocateEvents,
                        evm_->getIdentifier("ruProxyAllocateEvents") );
}


evb::evm::RUproxy::~RUproxy()
{
  for ( WorkLoops::iterator it = workLoops_.begin(), itEnd = workLoops_.end();
        it != itEnd; ++it)
  {
    if ( (*it)->isActive() )
      (*it)->cancel();
  }
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

  processRequestsWL_->submit(processRequestsAction_);
  for (uint32_t i=0; i < numberOfAllocators_; ++i)
  {
    workLoops_.at(i)->submit(allocateAction_);
  }
}


void evb::evm::RUproxy::drain() const
{
  while ( !isEmpty() ) ::usleep(1000);
}


bool evb::evm::RUproxy::isEmpty() const
{
  if ( ! readoutMsgFIFO_.empty() ) return false;

  AllocateFIFOs::const_iterator it = allocateFIFOs_.begin();
  while ( it != allocateFIFOs_.end() )
  {
    if ( ! (*it)->empty() ) return false;
    ++it;
  }

  return ( !processingActive_ && allocateActive_.none() );
}


void evb::evm::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while ( processingActive_ || allocateActive_.any() ) ::usleep(1000);

  readoutMsgFIFO_.clear();
  for (AllocateFIFOs::iterator it = allocateFIFOs_.begin(), itEnd = allocateFIFOs_.end();
       it != itEnd; ++it)
  {
    (*it)->clear();
  }
}


toolbox::mem::Reference* evb::evm::RUproxy::getRequestMsgBuffer(const uint32_t blockSize)
{
  toolbox::mem::Reference* rqstBufRef = 0;
  while ( !rqstBufRef )
  {
    try
    {
      rqstBufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_, blockSize);
    }
    catch(toolbox::mem::exception::Exception)
    {
      rqstBufRef = 0;
      if ( ! doProcessing_ )
        throw exception::HaltRequested();
    }
  }
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

  const uint32_t blockSize = evm_->getConfiguration()->blockSize;
  toolbox::mem::Reference* rqstBufRef = 0;
  readoutunit::FragmentRequestPtr fragmentRequest;
  uint32_t tries = 0;
  processingActive_ = true;

  try
  {
    rqstBufRef = getRequestMsgBuffer(blockSize);
    uint32_t msgSize = sizeof(msg::ReadoutMsg);
    unsigned char* payload = ((unsigned char*)rqstBufRef->getDataLocation()) + sizeof(msg::ReadoutMsg);

    while ( doProcessing_ )
    {
      if ( readoutMsgFIFO_.deq(fragmentRequest) )
      {
        const uint16_t requestsCount = fragmentRequest->nbRequests;
        const uint16_t ruCount = fragmentRequest->ruTids.size();
        const size_t readoutMsgSize = sizeof(msg::ReadoutMsg) +
          requestsCount * sizeof(EvBid) +
          ((ruCount+1)&~1) * sizeof(I2O_TID); // even number of I2O_TIDs to align header to 64-bits

        if ( msgSize+readoutMsgSize > blockSize )
        {
          enqueueMsg(rqstBufRef,msgSize);
          rqstBufRef = getRequestMsgBuffer(blockSize);
          msgSize = sizeof(msg::ReadoutMsg);
          payload = ((unsigned char*)rqstBufRef->getDataLocation()) + sizeof(msg::ReadoutMsg);
          tries = 0;
        }

        msg::EventRequest* eventRequest = (msg::EventRequest*)payload;
        eventRequest->msgSize      = readoutMsgSize;
        eventRequest->buTid        = fragmentRequest->buTid;
        eventRequest->priority     = 0; //not used on RU
        eventRequest->buResourceId = fragmentRequest->buResourceId;
        eventRequest->nbRequests   = requestsCount;
        eventRequest->nbDiscards   = fragmentRequest->nbDiscards;
        eventRequest->nbRUtids     = ruCount;

        payload = (unsigned char*)&eventRequest->evbIds[0];
        memcpy(payload,&fragmentRequest->evbIds[0],requestsCount*sizeof(EvBid));
        payload += requestsCount*sizeof(EvBid);

        memcpy(payload,&fragmentRequest->ruTids[0],ruCount*sizeof(I2O_TID));
        payload += ruCount*sizeof(I2O_TID);
        msgSize += readoutMsgSize;

        {
          boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

          allocateMonitoring_.lastEventNumberToRUs = fragmentRequest->evbIds[requestsCount-1].eventNumber();
          allocateMonitoring_.perf.logicalCount += requestsCount*ruCount_;
        }
      }
      else if ( doProcessing_ && tries < 100 )
      {
        ++tries;
        processingActive_ = false;
        ::usleep(100);
        processingActive_ = true;
      }
      else if ( rqstBufRef )
      {
        // send what we have so far
        enqueueMsg(rqstBufRef,msgSize);
        rqstBufRef = getRequestMsgBuffer(blockSize);
        msgSize = sizeof(msg::ReadoutMsg);
        payload = ((unsigned char*)rqstBufRef->getDataLocation()) + sizeof(msg::ReadoutMsg);
        tries = 0;
      }
    }
  }
  catch(exception::HaltRequested& e)
  {
    if (rqstBufRef) rqstBufRef->release();
    processingActive_ = false;
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

  return doProcessing_;
}


void evb::evm::RUproxy::enqueueMsg(toolbox::mem::Reference* rqstBufRef, const uint32_t msgSize)
{
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;

  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  stdMsg->MessageSize      = msgSize >> 2;
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
  rqstBufRef->setDataSize(msgSize);

  for (AllocateFIFOs::iterator it = allocateFIFOs_.begin(), itEnd = allocateFIFOs_.end();
       it != itEnd; ++it)
  {
    (*it)->enqWait( rqstBufRef->duplicate() );
  }
  rqstBufRef->release();

  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

    const uint32_t totalSize = msgSize*ruCount_;
    allocateMonitoring_.perf.sumOfSizes += totalSize;
    allocateMonitoring_.perf.sumOfSquares += totalSize*totalSize;
    allocateMonitoring_.perf.i2oCount += ruCount_;
  }
}


bool evb::evm::RUproxy::allocateEvents(toolbox::task::WorkLoop* wl)
{
  if ( ! doProcessing_ ) return false;

  const std::string wlName =  wl->getName();
  const size_t startPos = wlName.find_last_of("_") + 1;
  const size_t endPos = wlName.find("/",startPos);
  const uint16_t id = boost::lexical_cast<uint16_t>( wlName.substr(startPos,endPos-startPos) );

  // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  const uint32_t ruCountPerThread = (ruCount_ + numberOfAllocators_ - 1) / numberOfAllocators_;
  const ApplicationDescriptorsAndTids::const_iterator first = participatingRUs_.begin() + (ruCountPerThread*id);
  const ApplicationDescriptorsAndTids::const_iterator last = (id == numberOfAllocators_-1) ?
    participatingRUs_.end() : participatingRUs_.begin() + (ruCountPerThread*(id+1));

  try
  {
    while ( doProcessing_ )
    {
      {
        boost::mutex::scoped_lock sl(allocateActiveMutex_);
        allocateActive_.set(id);
      }

      toolbox::mem::Reference* fragmentRequestBufRef;
      while ( allocateFIFOs_[id]->deq(fragmentRequestBufRef) )
      {
        for (ApplicationDescriptorsAndTids::const_iterator it = first; it != last; ++it)
        {
          const uint32_t bufSize = fragmentRequestBufRef->getDataSize();
          toolbox::mem::Reference* rqstBufRef = getRequestMsgBuffer(bufSize);
          void* payload = rqstBufRef->getDataLocation();
          memcpy(payload,fragmentRequestBufRef->getDataLocation(),bufSize);
          I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)payload;
          stdMsg->TargetAddress = it->tid;

          {
            // Send the readout message to all RUs
            boost::mutex::scoped_lock sl(postFrameMutex_);

            try
            {
              evm_->getApplicationContext()->
                postFrame(
                  rqstBufRef,
                  evm_->getApplicationDescriptor(),
                  it->descriptor
                );
            }
            catch(xcept::Exception& e)
            {
              std::ostringstream msg;
              msg << "Failed to send message to RU ";
              msg << it->tid;
              XCEPT_RETHROW(exception::I2O, msg.str(), e);
            }
          }
        }
        fragmentRequestBufRef->release();
      }

      {
        boost::mutex::scoped_lock sl(allocateActiveMutex_);
        allocateActive_.reset(id);
      }
      ::usleep(100);
    }
  }
  catch(exception::HaltRequested& e)
  {
    boost::mutex::scoped_lock sl(allocateActiveMutex_);
    allocateActive_.reset(id);
  }
  catch(xcept::Exception& e)
  {
    {
      boost::mutex::scoped_lock sl(allocateActiveMutex_);
      allocateActive_.reset(id);
    }
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    {
      boost::mutex::scoped_lock sl(allocateActiveMutex_);
      allocateActive_.reset(id);
    }
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    {
      boost::mutex::scoped_lock sl(allocateActiveMutex_);
      allocateActive_.reset(id);
    }
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  return doProcessing_;
}


void evb::evm::RUproxy::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

  const double deltaT = allocateMonitoring_.perf.deltaT();
  allocateMonitoring_.bandwidth = allocateMonitoring_.perf.bandwidth(deltaT);
  allocateMonitoring_.assignmentRate = allocateMonitoring_.perf.logicalRate(deltaT);
  allocateMonitoring_.i2oRate = allocateMonitoring_.perf.i2oRate(deltaT);
  allocateMonitoring_.packingFactor = allocateMonitoring_.perf.packingFactor();
  allocateMonitoring_.perf.reset();
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

  numberOfAllocators_ = std::min(
    evm_->getConfiguration()->numberOfAllocators.value_,
    ruCount_
  );
  allocateFIFOs_.clear();
  allocateFIFOs_.reserve(numberOfAllocators_);
  for ( uint32_t i=0; i < numberOfAllocators_; ++i)
  {
    std::ostringstream fifoName;
    fifoName << "allocateFIFO_" << i;
    AllocateFIFOPtr allocateFIFO( new AllocateFIFO(evm_,fifoName.str()) );
    allocateFIFO->resize(evm_->getConfiguration()->allocateFIFOCapacity);
    allocateFIFOs_.push_back(allocateFIFO);
  }

  createProcessingWorkLoops();
}


void evb::evm::RUproxy::createProcessingWorkLoops()
{
  {
    boost::mutex::scoped_lock sl(allocateActiveMutex_);
    allocateActive_.clear();
    allocateActive_.resize(numberOfAllocators_);
  }

  const std::string identifier = evm_->getIdentifier();

  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=workLoops_.size(); i < numberOfAllocators_; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "/Allocator_" << i;
      toolbox::task::WorkLoop* wl = toolbox::task::getWorkLoopFactory()->getWorkLoop( workLoopName.str(), "waiting" );

      if ( ! wl->isActive() ) wl->activate();
      workLoops_.push_back(wl);
    }
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::WorkLoop, "Failed to start workloops", e);
  }
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
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(2);
      str << allocateMonitoring_.bandwidth / 1e3;
      table.add(tr()
                .add(td("throughput (kB/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << allocateMonitoring_.assignmentRate;
      table.add(tr()
                .add(td("assignment rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << allocateMonitoring_.i2oRate;
      table.add(tr()
                .add(td("I2O rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << allocateMonitoring_.packingFactor;
      table.add(tr()
                .add(td("Events assigned/I2O"))
                .add(td(str.str())));
    }

    const uint32_t activeRUs = ruTids_.empty() ? 0 : ruTids_.size()-1; //exclude the EVM TID
    table.add(tr()
              .add(td("# active RUs"))
              .add(td(boost::lexical_cast<std::string>(activeRUs))));

    uint32_t activeAllocators = 0;
    {
      boost::mutex::scoped_lock sl(allocateActiveMutex_);
      activeAllocators = allocateActive_.count();
    }
    table.add(tr()
              .add(td("# allocators active"))
              .add(td(boost::lexical_cast<std::string>(activeAllocators))));

    div.add(table);
  }
  for (AllocateFIFOs::const_iterator it = allocateFIFOs_.begin(), itEnd = allocateFIFOs_.end();
       it != itEnd; ++it)
  {
    div.add((*it)->getHtmlSnipped());
  }

  return div;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
