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
  numberOfAllocators_(0),
  doProcessing_(false),
  tid_(0)
{
  resetMonitoringCounters();

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


void evb::evm::RUproxy::startProcessing()
{
  resetMonitoringCounters();
  if ( participatingRUs_.empty() ) return;

  doProcessing_ = true;

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
  AllocateFIFOs::const_iterator it = allocateFIFOs_.begin();
  while ( it != allocateFIFOs_.end() )
  {
    if ( ! (*it)->empty() ) return false;
    ++it;
  }

  return ( allocateActive_.none() );
}


void evb::evm::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while ( allocateActive_.any() ) ::usleep(1000);

  for (AllocateFIFOs::iterator it = allocateFIFOs_.begin(), itEnd = allocateFIFOs_.end();
       it != itEnd; ++it)
  {
    (*it)->clear();
  }
}


toolbox::mem::Reference* evb::evm::RUproxy::getReadoutMsgBuffer(const uint32_t readoutMsgSize)
{
  toolbox::mem::Reference* rqstBufRef = 0;
  while ( !rqstBufRef )
  {
    try
    {
      rqstBufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_, readoutMsgSize);
    }
    catch(toolbox::mem::exception::Exception)
    {
      rqstBufRef = 0;
      if ( ! doProcessing_ ) return 0;
    }
  }
  rqstBufRef->setDataSize(readoutMsgSize);
  return rqstBufRef;
}


void evb::evm::RUproxy::sendRequest(const readoutunit::FragmentRequestPtr& fragmentRequest)
{
  if ( participatingRUs_.empty() ) return;

  const uint16_t requestsCount = fragmentRequest->nbRequests;
  const uint16_t ruCount = fragmentRequest->ruTids.size();
  const size_t readoutMsgSize = sizeof(msg::ReadoutMsg) +
    requestsCount * sizeof(EvBid) +
    ((ruCount+1)&~1) * sizeof(I2O_TID); // even number of I2O_TIDs to align header to 64-bits

  toolbox::mem::Reference* fragmentRequestBufRef = getReadoutMsgBuffer(readoutMsgSize);
  if ( ! fragmentRequestBufRef ) return;

  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)fragmentRequestBufRef->getDataLocation();
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  msg::ReadoutMsg* readoutMsg = (msg::ReadoutMsg*)stdMsg;

  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  stdMsg->MessageSize      = readoutMsgSize >> 2;
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
  readoutMsg->headerSize   = readoutMsgSize;
  readoutMsg->buTid        = fragmentRequest->buTid;
  readoutMsg->priority     = 0; //not used on RU
  readoutMsg->buResourceId = fragmentRequest->buResourceId;
  readoutMsg->nbRequests   = requestsCount;
  readoutMsg->nbDiscards   = fragmentRequest->nbDiscards;
  readoutMsg->nbRUtids     = ruCount;

  unsigned char* payload = (unsigned char*)&readoutMsg->evbIds[0];
  memcpy(payload,&fragmentRequest->evbIds[0],requestsCount*sizeof(EvBid));
  payload += requestsCount*sizeof(EvBid);

  memcpy(payload,&fragmentRequest->ruTids[0],ruCount*sizeof(I2O_TID));

  for (AllocateFIFOs::iterator it = allocateFIFOs_.begin(), itEnd = allocateFIFOs_.end();
       it != itEnd; ++it)
  {
    (*it)->enqWait( fragmentRequestBufRef->duplicate() );
  }
  fragmentRequestBufRef->release();

  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

    const uint32_t msgSize = readoutMsgSize*ruCount;
    allocateMonitoring_.lastEventNumberToRUs = fragmentRequest->evbIds[requestsCount-1].eventNumber();
    allocateMonitoring_.perf.sumOfSizes += msgSize;
    allocateMonitoring_.perf.sumOfSquares += msgSize*msgSize;
    allocateMonitoring_.perf.logicalCount += requestsCount*ruCount;
    allocateMonitoring_.perf.i2oCount += ruCount;
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
  const uint32_t ruCountPerThread = (participatingRUs_.size() + numberOfAllocators_ - 1) / numberOfAllocators_;
  const ApplicationDescriptorsAndTids::const_iterator first = participatingRUs_.begin() + (ruCountPerThread*id);
  const ApplicationDescriptorsAndTids::const_iterator last = (id == numberOfAllocators_-1) ?
    participatingRUs_.end() : participatingRUs_.begin() + (ruCountPerThread*(id+1));

  while ( doProcessing_ )
  {
    try
    {
      {
        boost::mutex::scoped_lock sl(allocateActiveMutex_);
        allocateActive_.set(id);
      }

      readoutunit::FragmentRequestPtr fragmentRequest;
      while ( allocateFIFOs_[id]->deq(fragmentRequest) )

      toolbox::mem::Reference* fragmentRequestBufRef;
      while ( allocateFIFOs_[id]->deq(fragmentRequestBufRef) )
      {
        for (ApplicationDescriptorsAndTids::const_iterator it = first; it != last; ++it)
        {
          const uint32_t bufSize = fragmentRequestBufRef->getDataSize();
          toolbox::mem::Reference* rqstBufRef = getReadoutMsgBuffer(bufSize);
          if ( ! rqstBufRef )
          {
            for (BufRefsAndDescs::iterator it = bufRefsAndDescs.begin(), itEnd = bufRefsAndDescs.end(); it != itEnd; ++it)
            {
              it->first->release();
              bufRefsAndDescs.clear();
            }
            boost::mutex::scoped_lock sl(allocateActiveMutex_);
            allocateActive_.reset(id);
            return false;
          }

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

  numberOfAllocators_ = std::min(
    evm_->getConfiguration()->numberOfAllocators.value_,
    static_cast<uint32_t>(participatingRUs_.size())
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
