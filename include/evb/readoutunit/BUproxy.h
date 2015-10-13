#ifndef _evb_readoutunit_BUproxy_h_
#define _evb_readoutunit_BUproxy_h_

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <set>
#include <stdint.h>
#include <vector>

#include "cgicc/HTMLClasses.h"
#include "evb/Constants.h"
#include "evb/DataLocations.h"
#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/readoutunit/BUposter.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/FragmentRequest.h"
#include "evb/readoutunit/StateMachine.h"
#include "evb/readoutunit/SuperFragment.h"
#include "i2o/i2oDdmLib.h"
#include "i2o/Method.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "interface/shared/i2ogevb2g.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/WorkLoop.h"
#include "xcept/tools.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {

  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Proxy for RU-BU communication
     */

    template<class ReadoutUnit>
    class BUproxy : public toolbox::lang::Class
    {

    public:

      BUproxy(ReadoutUnit*);

      ~BUproxy();

      /**
       * Callback for fragment request I2O message from BU/EVM
       */
      void readoutMsgCallback(toolbox::mem::Reference*);

      /**
       * Configure the BU proxy
       */
      void configure();

      /**
       * Append the info space items to be published in the
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);

      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();

      /**
       * Start processing events
       */
      void startProcessing();

      /**
       * Drain events
       */
      void drain();

      /**
       * Stop processing events
       */
      void stopProcessing();

      /**
       * Return the latest lumi section sent to the BUs
       */
      uint32_t getLatestLumiSection() const;

      /**
       * Return the number of events built so far
       */
      uint64_t getNbEventsBuilt() const;

      /**
       * Return the number of events built so far
       */
      uint64_t getFragmentCount() const;

      /**
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;


    private:

      typedef std::vector<SuperFragmentPtr> SuperFragments;

      void resetMonitoringCounters();
      void startProcessingWorkLoop();
      void createProcessingWorkLoops();
      void updateRequestCounters(const FragmentRequestPtr&);
      bool process(toolbox::task::WorkLoop*);
      bool processRequest(FragmentRequestPtr&,SuperFragments&);
      void handleRequest(const msg::ReadoutMsg*, FragmentRequestPtr&);
      void sendData(const FragmentRequestPtr&, const SuperFragments&);
      toolbox::mem::Reference* getNextBlock(const uint32_t blockNb) const;
      void fillSuperFragmentHeader
      (
        unsigned char*& payload,
        uint32_t& remainingPayloadSize,
        const uint32_t superFragmentNb,
        const SuperFragmentPtr& superFragment,
        const uint32_t currentFragmentSize
      ) const;
      bool isEmpty();
      cgicc::table getStatisticsPerBU() const;
      std::string getHelpTextForBuRequests() const;

      ReadoutUnit* readoutUnit_;
      typename ReadoutUnit::InputPtr input_;
      BUposter<ReadoutUnit> buPoster_;
      I2O_TID tid_;
      toolbox::mem::Pool* superFragmentPool_;

      typedef std::vector<toolbox::task::WorkLoop*> WorkLoops;
      WorkLoops workLoops_;
      toolbox::task::ActionSignature* action_;
      volatile bool doProcessing_;
      boost::dynamic_bitset<> processesActive_;
      boost::mutex processesActiveMutex_;
      uint32_t nbActiveProcesses_;
      mutable boost::mutex processingRequestMutex_;

      //used on the RU
      typedef OneToOneQueue<FragmentRequestPtr> FragmentRequestFIFO;
      FragmentRequestFIFO fragmentRequestFIFO_;

      //used on the EVM
      typedef boost::shared_ptr<FragmentRequestFIFO> FragmentRequestFIFOPtr;
      typedef std::vector<FragmentRequestFIFOPtr> PrioritizedFragmentRequestFIFOs;
      typedef std::map<I2O_TID,PrioritizedFragmentRequestFIFOs> FragmentRequestFIFOs;
      FragmentRequestFIFOs::iterator nextBU_;
      FragmentRequestFIFOs fragmentRequestFIFOs_;
      mutable boost::shared_mutex fragmentRequestFIFOsMutex_;

      typedef std::map<I2O_TID,uint64_t> CountsPerBU;
      struct RequestMonitoring
      {
        uint64_t requestCount;
        uint64_t bandwidth;
        uint32_t requestRate;
        uint32_t i2oRate;
        double packingFactor;
        PerformanceMonitor perf;
        CountsPerBU logicalCountPerBU;
      } requestMonitoring_;
      mutable boost::mutex requestMonitoringMutex_;

      struct DataMonitoring
      {
        uint32_t lastEventNumberToBUs;
        uint32_t lastLumiSectionToBUs;
        int32_t outstandingEvents;
        uint64_t fragmentCount;
        uint64_t nbEventsBuilt;
        uint64_t bandwidth;
        uint32_t fragmentRate;
        uint32_t i2oRate;
        double packingFactor;
        PerformanceMonitor perf;
        CountsPerBU payloadPerBU;
      } dataMonitoring_;
      mutable boost::mutex dataMonitoringMutex_;

      xdata::UnsignedInteger32 activeRequests_;
      xdata::UnsignedInteger64 requestCount_;
      xdata::UnsignedInteger64 fragmentCount_;
      xdata::UnsignedInteger64 nbEventsBuilt_;
      xdata::Vector<xdata::UnsignedInteger64> requestCountPerBU_;
      xdata::Vector<xdata::UnsignedInteger64> payloadPerBU_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit>
evb::readoutunit::BUproxy<ReadoutUnit>::BUproxy(ReadoutUnit* readoutUnit) :
readoutUnit_(readoutUnit),
buPoster_(readoutUnit),
tid_(0),
doProcessing_(false),
nbActiveProcesses_(0),
fragmentRequestFIFO_(readoutUnit,"fragmentRequestFIFO")
{
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool",readoutUnit_->getConfiguration()->sendPoolName);
    superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch(toolbox::mem::exception::MemoryPoolNotFound)
  {
    toolbox::net::URN poolURN("toolbox-mem-pool","superFragment");
    try
    {
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        createPool(poolURN, new toolbox::mem::HeapAllocator());
    }
    catch(toolbox::mem::exception::DuplicateMemoryPool)
    {
      // Pool already exists from a previous construction of this class
      // Note that destroying the pool in the destructor is not working
      // because the previous instance is destroyed after the new one
      // is constructed.
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        findPool(poolURN);
    }
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for super-fragments", e);
  }

  resetMonitoringCounters();
  startProcessingWorkLoop();
}


template<class ReadoutUnit>
evb::readoutunit::BUproxy<ReadoutUnit>::~BUproxy()
{
  for ( WorkLoops::iterator it = workLoops_.begin(), itEnd = workLoops_.end();
        it != itEnd; ++it)
  {
    if ( (*it)->isActive() )
      (*it)->cancel();
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::startProcessingWorkLoop()
{
  action_ = toolbox::task::bind(this, &evb::readoutunit::BUproxy<ReadoutUnit>::process,
                                readoutUnit_->getIdentifier("process") );
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::startProcessing()
{
  resetMonitoringCounters();
  buPoster_.startProcessing();

  doProcessing_ = true;

  for (uint32_t i=0; i < readoutUnit_->getConfiguration()->numberOfResponders; ++i)
  {
    workLoops_.at(i)->submit(action_);
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::drain()
{
  while ( ! isEmpty() ) ::usleep(1000);
  buPoster_.drain();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::stopProcessing()
{
  doProcessing_ = false;
  buPoster_.stopProcessing();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::readoutMsgCallback(toolbox::mem::Reference* bufRef)
{
  msg::ReadoutMsg* readoutMsg =
    (msg::ReadoutMsg*)bufRef->getDataLocation();

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    dataMonitoring_.outstandingEvents -= readoutMsg->nbDiscards;
  }

  if ( readoutMsg->nbRequests > 0 )
  {
    FragmentRequestPtr fragmentRequest( new FragmentRequest );
    fragmentRequest->buTid = readoutMsg->buTid;
    fragmentRequest->buResourceId = readoutMsg->buResourceId;
    fragmentRequest->nbRequests = readoutMsg->nbRequests;
    handleRequest(readoutMsg, fragmentRequest);

    updateRequestCounters(fragmentRequest);
  }

  bufRef->release();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::updateRequestCounters(const FragmentRequestPtr& fragmentRequest)
{
  boost::mutex::scoped_lock sl(requestMonitoringMutex_);

  uint32_t msgSize = sizeof(msg::ReadoutMsg) +
    ((fragmentRequest->ruTids.size()+1)&~1) * sizeof(I2O_TID); // even number of I2O_TIDs to align header to 64-bits
  if ( !fragmentRequest->evbIds.empty() )
    msgSize += fragmentRequest->nbRequests * sizeof(EvBid);

  requestMonitoring_.perf.sumOfSizes += msgSize;
  requestMonitoring_.perf.sumOfSquares += msgSize*msgSize;
  requestMonitoring_.perf.logicalCount += fragmentRequest->nbRequests;
  ++requestMonitoring_.perf.i2oCount;
  requestMonitoring_.logicalCountPerBU[fragmentRequest->buTid] += fragmentRequest->nbRequests;
}


template<class ReadoutUnit>
bool evb::readoutunit::BUproxy<ReadoutUnit>::process(toolbox::task::WorkLoop* wl)
{
  if ( ! doProcessing_ ) return false;

  const std::string wlName =  wl->getName();
  const size_t startPos = wlName.find_last_of("_") + 1;
  const size_t endPos = wlName.find("/",startPos);
  const uint16_t responderId = boost::lexical_cast<uint16_t>( wlName.substr(startPos,endPos-startPos) );

  {
    boost::mutex::scoped_lock sl(processesActiveMutex_);
    processesActive_.set(responderId);
  }

  try
  {
    FragmentRequestPtr fragmentRequest;
    SuperFragments superFragments;

    while ( processRequest(fragmentRequest,superFragments) )
    {
      sendData(fragmentRequest, superFragments);
      superFragments.clear();
    }
  }
  catch(xcept::Exception& e)
  {
    {
      boost::mutex::scoped_lock sl(processesActiveMutex_);
      processesActive_.reset(responderId);
    }
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    {
      boost::mutex::scoped_lock sl(processesActiveMutex_);
      processesActive_.reset(responderId);
    }
    XCEPT_DECLARE(exception::SuperFragment,
                  sentinelException, e.what());
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    {
      boost::mutex::scoped_lock sl(processesActiveMutex_);
      processesActive_.reset(responderId);
    }
    XCEPT_DECLARE(exception::SuperFragment,
                  sentinelException, "unkown exception");
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  {
    boost::mutex::scoped_lock sl(processesActiveMutex_);
    processesActive_.reset(responderId);
  }

  ::usleep(10);

  return doProcessing_;
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::sendData
(
  const FragmentRequestPtr& fragmentRequest,
  const SuperFragments& superFragments
)
{
  uint32_t blockNb = 1;
  const uint16_t nbSuperFragments = superFragments.size();
  assert( nbSuperFragments == fragmentRequest->evbIds.size() );
  assert( nbSuperFragments > 0 );
  const uint16_t nbRUtids = fragmentRequest->ruTids.size();

  toolbox::mem::Reference* head = getNextBlock(blockNb);
  toolbox::mem::Reference* tail = head;

  const uint32_t blockHeaderSize = sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME)
    + nbSuperFragments * sizeof(EvBid)
    + ((nbRUtids+1)&~1) * sizeof(I2O_TID); // always have an even number of 32-bit I2O_TIDs to keep 64-bit alignment

  assert( blockHeaderSize % 8 == 0 );
  assert( blockHeaderSize < readoutUnit_->getConfiguration()->blockSize );
  unsigned char* payload = (unsigned char*)head->getDataLocation() + blockHeaderSize;
  uint32_t remainingPayloadSize = readoutUnit_->getConfiguration()->blockSize - blockHeaderSize;

  for (uint32_t i=0; i < nbSuperFragments; ++i)
  {
    const SuperFragmentPtr superFragment = superFragments[i];
    uint32_t remainingSuperFragmentSize = superFragment->getSize();

    fillSuperFragmentHeader(payload,remainingPayloadSize,i+1,superFragment,remainingSuperFragmentSize);

    const SuperFragment::FedFragments& fedFragments = superFragment->getFedFragments();
    for ( SuperFragment::FedFragments::const_iterator it = fedFragments.begin(), itEnd = fedFragments.end();
          it != itEnd; ++it)
    {
      const DataLocations& fedData = (*it)->getDataLocations();
      for ( DataLocations::const_iterator it = fedData.begin(), itEnd = fedData.end();
            it != itEnd; ++it)
      {
        const unsigned char* chunkBase  = (unsigned char*)it->iov_base;
        uint32_t chunkSize = it->iov_len;
        uint32_t copiedSize = 0;

        while ( chunkSize > remainingPayloadSize )
        {
          // fill the remaining block
          memcpy(payload, chunkBase + copiedSize, remainingPayloadSize);
          copiedSize += remainingPayloadSize;
          chunkSize -= remainingPayloadSize;
          remainingSuperFragmentSize -= remainingPayloadSize;

          // get a new block
          toolbox::mem::Reference* nextBlock = getNextBlock(++blockNb);
          payload = (unsigned char*)nextBlock->getDataLocation() + sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
          remainingPayloadSize = readoutUnit_->getConfiguration()->blockSize - sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
          fillSuperFragmentHeader(payload,remainingPayloadSize,i+1,superFragment,remainingSuperFragmentSize);
          tail->setNextReference(nextBlock);
          tail = nextBlock;
        }

        // fill the remaining fragment into the block
        memcpy(payload, chunkBase + copiedSize, chunkSize);
        payload += chunkSize;
        remainingPayloadSize -= chunkSize;
        remainingSuperFragmentSize -= chunkSize;
      }

      const fedt_t* trailer = (fedt_t*)(payload - sizeof(fedt_t));
      assert ( FED_TCTRLID_EXTRACT(trailer->eventsize) == FED_SLINK_END_MARKER );
    }
  }
  tail->setDataSize( readoutUnit_->getConfiguration()->blockSize - remainingPayloadSize );

  toolbox::mem::Reference* bufRef = head;
  uint32_t payloadSize = 0;
  uint32_t i2oCount = 0;
  uint32_t lastEventNumberToBUs = 0;
  uint32_t lastLumiSectionToBUs = 0;


  // Prepare each event data block for the BU
  while (bufRef)
  {
    toolbox::mem::Reference* nextRef = bufRef->getNextReference();
    bufRef->setNextReference(0);

    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg = (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;

    stdMsg->VersionOffset          = 0;
    stdMsg->MsgFlags               = 0;
    stdMsg->MessageSize            = bufRef->getDataSize() >> 2;
    stdMsg->InitiatorAddress       = tid_;
    stdMsg->TargetAddress          = fragmentRequest->buTid;
    stdMsg->Function               = I2O_PRIVATE_MESSAGE;
    pvtMsg->OrganizationID         = XDAQ_ORGANIZATION_ID;
    pvtMsg->XFunctionCode          = I2O_BU_CACHE;
    dataBlockMsg->buResourceId     = fragmentRequest->buResourceId;
    dataBlockMsg->nbBlocks         = blockNb;
    dataBlockMsg->nbSuperFragments = nbSuperFragments;
    dataBlockMsg->nbRUtids         = nbRUtids;

    if ( dataBlockMsg->blockNb == 1 )
    {
      dataBlockMsg->headerSize     = blockHeaderSize;

      unsigned char* payload = (unsigned char*)&dataBlockMsg->evbIds[0];
      size_t size = nbSuperFragments*sizeof(EvBid);
      memcpy(payload,&fragmentRequest->evbIds[0],size);
      payload += size;
      lastEventNumberToBUs = fragmentRequest->evbIds[nbSuperFragments-1].eventNumber();
      lastLumiSectionToBUs = fragmentRequest->evbIds[nbSuperFragments-1].lumiSection();

      size = nbRUtids*sizeof(I2O_TID);
      memcpy(payload,&fragmentRequest->ruTids[0],size);
    }
    else
    {
      dataBlockMsg->headerSize     = sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
    }

    payloadSize += bufRef->getDataSize();
    ++i2oCount;

    buPoster_.sendFrame(fragmentRequest->buTid,bufRef);
    bufRef = nextRef;
  }

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);

    dataMonitoring_.lastEventNumberToBUs = lastEventNumberToBUs;
    dataMonitoring_.lastLumiSectionToBUs = lastLumiSectionToBUs;
    dataMonitoring_.outstandingEvents += nbSuperFragments;
    dataMonitoring_.perf.i2oCount += i2oCount;
    dataMonitoring_.perf.sumOfSizes += payloadSize;
    dataMonitoring_.perf.sumOfSquares += payloadSize*payloadSize;
    dataMonitoring_.perf.logicalCount += nbSuperFragments;
    dataMonitoring_.payloadPerBU[fragmentRequest->buTid] += payloadSize;
  }
}


template<class ReadoutUnit>
toolbox::mem::Reference* evb::readoutunit::BUproxy<ReadoutUnit>::getNextBlock
(
  const uint32_t blockNb
) const
{
  toolbox::mem::Reference* bufRef =
    toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,readoutUnit_->getConfiguration()->blockSize);
  bufRef->setDataSize(readoutUnit_->getConfiguration()->blockSize);

  msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg = (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  dataBlockMsg->blockNb = blockNb;

  return bufRef;
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::fillSuperFragmentHeader
(
  unsigned char*& payload,
  uint32_t& remainingPayloadSize,
  const uint32_t superFragmentNb,
  const SuperFragmentPtr& superFragment,
  const uint32_t currentFragmentSize
) const
{
  const SuperFragment::MissingFedIds& missingFedIds = superFragment->getMissingFedIds();
  const uint16_t nbDroppedFeds = missingFedIds.size();
  const uint32_t headerSize = sizeof(msg::SuperFragment)
    + ((nbDroppedFeds-1 + 3) / 4) * sizeof(uint64_t);
  // Keep 64-bit alignment if nbDroppedFeds > 1.
  // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  assert( headerSize % 8 == 0 );

  if ( remainingPayloadSize < headerSize )
  {
    remainingPayloadSize = 0;
    return;
  }

  msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;

  payload += headerSize;
  remainingPayloadSize -= headerSize;

  superFragmentMsg->headerSize = headerSize;
  superFragmentMsg->superFragmentNb = superFragmentNb;
  superFragmentMsg->totalSize = superFragment->getSize();
  superFragmentMsg->partSize = currentFragmentSize > remainingPayloadSize ? remainingPayloadSize : currentFragmentSize;
  superFragmentMsg->nbDroppedFeds = nbDroppedFeds;

  memcpy(&superFragmentMsg->fedIds[0],&missingFedIds[0],nbDroppedFeds*sizeof(uint16_t));
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::configure()
{
  {
     boost::unique_lock<boost::shared_mutex> ul(fragmentRequestFIFOsMutex_);

     fragmentRequestFIFO_.clear();
     fragmentRequestFIFO_.resize(readoutUnit_->getConfiguration()->fragmentRequestFIFOCapacity);
     fragmentRequestFIFOs_.clear();
     nextBU_ = fragmentRequestFIFOs_.begin();
  }

  if ( readoutUnit_->getConfiguration()->numberOfPreallocatedBlocks.value_ > 0 )
  {
    toolbox::mem::Reference* head =
      toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,readoutUnit_->getConfiguration()->blockSize);
    toolbox::mem::Reference* tail = head;
    for (uint32_t i = 1; i < readoutUnit_->getConfiguration()->numberOfPreallocatedBlocks; ++i)
    {
      toolbox::mem::Reference* bufRef =
        toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,readoutUnit_->getConfiguration()->blockSize);
      tail->setNextReference(bufRef);
      tail = bufRef;
    }
    head->release();
  }

  input_ = readoutUnit_->getInput();

  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(readoutUnit_->getApplicationDescriptor());
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::I2O,
                  "Failed to get I2O TID for this application", e);
  }

  createProcessingWorkLoops();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::createProcessingWorkLoops()
{
  processesActive_.clear();
  processesActive_.resize(readoutUnit_->getConfiguration()->numberOfResponders);

  const std::string identifier = readoutUnit_->getIdentifier();

  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=workLoops_.size(); i < readoutUnit_->getConfiguration()->numberOfResponders; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "/Responder_" << i;
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


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::appendMonitoringItems(InfoSpaceItems& items)
{
  activeRequests_ = 0;
  requestCount_ = 0;
  fragmentCount_ = 0;
  nbEventsBuilt_ = 0;
  requestCountPerBU_.clear();
  payloadPerBU_.clear();

  items.add("activeRequests", &activeRequests_);
  items.add("requestCount", &requestCount_);
  items.add("fragmentCount", &fragmentCount_);
  items.add("nbEventsBuilt", &nbEventsBuilt_);
  items.add("requestCountPerBU", &requestCountPerBU_);
  items.add("payloadPerBU", &payloadPerBU_);
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::updateMonitoringItems()
{
  {
    boost::shared_lock<boost::shared_mutex> sl(fragmentRequestFIFOsMutex_);

    activeRequests_ = fragmentRequestFIFO_.elements();
    for (FragmentRequestFIFOs::const_iterator it = fragmentRequestFIFOs_.begin();
         it != fragmentRequestFIFOs_.end(); ++it)
    {
      for ( uint16_t priority = 0; priority <= evb::LOWEST_PRIORITY; ++priority)
      {
        activeRequests_.value_ += it->second[priority]->elements();
      }
    }
  }
  {
    boost::mutex::scoped_lock sl(processingRequestMutex_, boost::try_to_lock);
    if ( ! sl ) ++activeRequests_.value_;
  }
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);

    const double deltaT = requestMonitoring_.perf.deltaT();
    requestMonitoring_.requestCount += requestMonitoring_.perf.logicalCount;
    requestMonitoring_.bandwidth = requestMonitoring_.perf.bandwidth(deltaT);
    requestMonitoring_.requestRate = requestMonitoring_.perf.logicalRate(deltaT);
    requestMonitoring_.i2oRate = requestMonitoring_.perf.i2oRate(deltaT);
    requestMonitoring_.packingFactor = requestMonitoring_.perf.packingFactor();
    requestCount_ = requestMonitoring_.requestCount;

    requestCountPerBU_.clear();
    requestCountPerBU_.reserve(requestMonitoring_.logicalCountPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = requestMonitoring_.logicalCountPerBU.begin(),
           itEnd = requestMonitoring_.logicalCountPerBU.end();
         it != itEnd; ++it)
    {
      requestCountPerBU_.push_back(it->second);
    }
    requestMonitoring_.perf.reset();
  }
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);

    const double deltaT = dataMonitoring_.perf.deltaT();
    dataMonitoring_.fragmentCount += dataMonitoring_.perf.logicalCount;
    dataMonitoring_.nbEventsBuilt += dataMonitoring_.fragmentCount - dataMonitoring_.outstandingEvents;
    dataMonitoring_.bandwidth = dataMonitoring_.perf.bandwidth(deltaT);
    dataMonitoring_.fragmentRate = dataMonitoring_.perf.logicalRate(deltaT);
    dataMonitoring_.i2oRate = dataMonitoring_.perf.i2oRate(deltaT);
    dataMonitoring_.packingFactor = dataMonitoring_.perf.packingFactor();
    fragmentCount_ = dataMonitoring_.fragmentCount;
    nbEventsBuilt_ = dataMonitoring_.nbEventsBuilt;

    payloadPerBU_.clear();
    payloadPerBU_.reserve(dataMonitoring_.payloadPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = dataMonitoring_.payloadPerBU.begin(),
           itEnd = dataMonitoring_.payloadPerBU.end();
         it != itEnd; ++it)
    {
      payloadPerBU_.push_back(it->second);
    }
    dataMonitoring_.perf.reset();
  }
  {
    boost::mutex::scoped_lock sl(processesActiveMutex_);
    nbActiveProcesses_ = processesActive_.count();
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::resetMonitoringCounters()
{

  {
    boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
    requestMonitoring_.requestCount = 0;
    requestMonitoring_.perf.reset();
    requestMonitoring_.logicalCountPerBU.clear();
  }
  {
    boost::mutex::scoped_lock dsl(dataMonitoringMutex_);
    dataMonitoring_.lastEventNumberToBUs = 0;
    dataMonitoring_.lastLumiSectionToBUs = 0;
    dataMonitoring_.outstandingEvents = 0;
    dataMonitoring_.fragmentCount = 0;
    dataMonitoring_.nbEventsBuilt = 0;
    dataMonitoring_.perf.reset();
    dataMonitoring_.payloadPerBU.clear();
  }
}


template<class ReadoutUnit>
uint64_t evb::readoutunit::BUproxy<ReadoutUnit>::getNbEventsBuilt() const
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  return dataMonitoring_.fragmentCount + dataMonitoring_.perf.logicalCount - dataMonitoring_.outstandingEvents;
}


template<class ReadoutUnit>
uint64_t evb::readoutunit::BUproxy<ReadoutUnit>::getFragmentCount() const
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  return dataMonitoring_.fragmentCount + dataMonitoring_.perf.logicalCount;
}


template<class ReadoutUnit>
uint32_t evb::readoutunit::BUproxy<ReadoutUnit>::getLatestLumiSection() const
{
  boost::mutex::scoped_lock dsl(dataMonitoringMutex_);
  return dataMonitoring_.lastLumiSectionToBUs;
}


template<class ReadoutUnit>
cgicc::div evb::readoutunit::BUproxy<ReadoutUnit>::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("BUproxy"));

  {
    boost::mutex::scoped_lock dsl(dataMonitoringMutex_);

    table table;
    table.set("title","Super fragments are sent to the BUs requesting events.");

    table.add(tr()
              .add(td("last evt number to BUs"))
              .add(td(boost::lexical_cast<std::string>(dataMonitoring_.lastEventNumberToBUs))));
    table.add(tr()
              .add(td("last lumi section to BUs"))
              .add(td(boost::lexical_cast<std::string>(dataMonitoring_.lastLumiSectionToBUs))));
    table.add(tr()
              .add(td("# of events built"))
              .add(td(boost::lexical_cast<std::string>(dataMonitoring_.fragmentCount + dataMonitoring_.perf.logicalCount - dataMonitoring_.outstandingEvents))));
    table.add(tr()
              .add(td("# of active responders"))
              .add(td(boost::lexical_cast<std::string>(nbActiveProcesses_))));
    table.add(tr()
              .add(td("# of active requests"))
              .add(td(boost::lexical_cast<std::string>(activeRequests_.value_))));

    // outstanding events is negative for the RUs, but positive for the EVM
    table.add(tr()
              .add(td("# of outstanding events"))
              .add(td(boost::lexical_cast<std::string>(abs(dataMonitoring_.outstandingEvents)))));

    table.add(tr()
              .add(th("Event data").set("colspan","2")));
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(2);
      str << dataMonitoring_.bandwidth / 1e6;
      table.add(tr()
                .add(td("throughput (MB/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << dataMonitoring_.fragmentRate;
      table.add(tr()
                .add(td("fragment rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << dataMonitoring_.i2oRate;
      table.add(tr()
                .add(td("I2O rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << dataMonitoring_.packingFactor;
      table.add(tr()
                .add(td("Fragments/I2O"))
                .add(td(str.str())));
    }
    div.add(table);
  }

  {
    boost::mutex::scoped_lock rsl(requestMonitoringMutex_);

    table table;
    table.set("title",getHelpTextForBuRequests());

    table.add(tr()
              .add(th("Event requests").set("colspan","2")));
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(2);
      str << requestMonitoring_.bandwidth / 1e3;
      table.add(tr()
                .add(td("throughput (kB/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << requestMonitoring_.requestRate;
      table.add(tr()
                .add(td("request rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(0);
      str << requestMonitoring_.i2oRate;
      table.add(tr()
                .add(td("I2O rate (Hz)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << requestMonitoring_.packingFactor;
      table.add(tr()
                .add(td("Events requested/I2O"))
                .add(td(str.str())));
    }
    div.add(table);

    {
      boost::shared_lock<boost::shared_mutex> sl(fragmentRequestFIFOsMutex_);

      if ( fragmentRequestFIFOs_.empty() )
      {
        div.add(fragmentRequestFIFO_.getHtmlSnipped());
      }
      else
      {
        for (FragmentRequestFIFOs::const_iterator it = fragmentRequestFIFOs_.begin();
             it != fragmentRequestFIFOs_.end(); ++it)
        {
          uint16_t priority = 0;
          while ( priority <= evb::LOWEST_PRIORITY &&
                  it->second[priority]->empty() )
          {
            ++priority;
          }
          if ( priority > evb::LOWEST_PRIORITY )
            div.add(it->second[0]->getHtmlSnipped());
          else
            div.add(it->second[priority]->getHtmlSnipped());
        }
      }
    }
  }

  div.add(buPoster_.getPosterFIFOs());
  div.add(getStatisticsPerBU());

  return div;
}


template<class ReadoutUnit>
cgicc::table evb::readoutunit::BUproxy<ReadoutUnit>::getStatisticsPerBU() const
{
  using namespace cgicc;

  table table;
  table.set("title","Number of event requests and sum of payload per BU.");

  table.add(tr()
            .add(th("Statistics per BU").set("colspan","4")));
  table.add(tr()
            .add(td("Instance"))
            .add(td("TID"))
            .add(td("Nb requests"))
            .add(td("Data payload (MB)")));

  CountsPerBU::const_iterator it, itEnd;
  for (it=requestMonitoring_.logicalCountPerBU.begin(), itEnd = requestMonitoring_.logicalCountPerBU.end();
       it != itEnd; ++it)
  {
    const I2O_TID buTID = it->first;
    uint32_t payloadPerBU = 0;
    try
    {
      payloadPerBU = dataMonitoring_.payloadPerBU.at(buTID) / 1e6;
    }
    catch(std::out_of_range) {}

    xdaq::ApplicationDescriptor* bu = i2o::utils::getAddressMap()->getApplicationDescriptor(buTID);
    const std::string url = bu->getContextDescriptor()->getURL() + "/" + bu->getURN();
    table.add(tr()
              .add(td()
                   .add(a("BU "+boost::lexical_cast<std::string>(bu->getInstance())).set("href",url).set("target","_blank")))
              .add(td(boost::lexical_cast<std::string>(buTID)))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.logicalCountPerBU.at(buTID))))
              .add(td(boost::lexical_cast<std::string>(payloadPerBU))));
  }

  return table;
}

#endif // _evb_readoutunit_BUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
