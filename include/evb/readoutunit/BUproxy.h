#ifndef _evb_readoutunit_BUproxy_h_
#define _evb_readoutunit_BUproxy_h_

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <set>
#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/FragmentRequest.h"
#include "i2o/i2oDdmLib.h"
#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
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
       * Start processing events
       */
      void stopProcessing();

      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*) const;

      /**
       * Print the content of the fragment-request FIFO as HTML snipped
       */
      inline void printFragmentRequestFIFO(xgi::Output* out) const
      { fragmentRequestFIFO_.printVerticalHtml(out); }


    private:

      typedef std::vector<FragmentChainPtr> SuperFragments;

      void resetMonitoringCounters();
      void startProcessingWorkLoop();
      void createProcessingWorkLoops();
      void updateRequestCounters(const FragmentRequestPtr);
      bool process(toolbox::task::WorkLoop*);
      bool processRequest(FragmentRequestPtr&,SuperFragments&);
      void fillRequest(const msg::ReadoutMsg*, FragmentRequestPtr&) const;
      void sendData(const FragmentRequestPtr&, const SuperFragments&);
      toolbox::mem::Reference* getNextBlock(const uint32_t blockNb) const;
      void fillSuperFragmentHeader
      (
        unsigned char*& payload,
        uint32_t& remainingPayloadSize,
        const uint32_t superFragmentNb,
        const uint32_t superFragmentSize,
        const uint32_t currentFragmentSize
      ) const;

      ReadoutUnit* readoutUnit_;
      typename ReadoutUnit::InputPtr input_;
      const ConfigurationPtr configuration_;
      I2O_TID tid_;
      toolbox::mem::Pool* superFragmentPool_;

      typedef std::vector<toolbox::task::WorkLoop*> WorkLoops;
      WorkLoops workLoops_;
      toolbox::task::ActionSignature* action_;
      volatile bool doProcessing_;
      boost::dynamic_bitset<> processesActive_;
      boost::mutex processesActiveMutex_;

      typedef OneToOneQueue<FragmentRequestPtr> FragmentRequestFIFO;
      FragmentRequestFIFO fragmentRequestFIFO_;
      boost::mutex fragmentRequestFIFOmutex_;

      typedef std::map<uint32_t,uint64_t> CountsPerBU;
      struct RequestMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        CountsPerBU logicalCountPerBU;
      } requestMonitoring_;
      mutable boost::mutex requestMonitoringMutex_;

      struct DataMonitoring
      {
        uint32_t lastEventNumberToBUs;
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        CountsPerBU payloadPerBU;
      } dataMonitoring_;
      mutable boost::mutex dataMonitoringMutex_;

      xdata::UnsignedInteger32 lastEventNumberToBUs_;
      xdata::UnsignedInteger64 i2oBUCacheCount_;
      xdata::Vector<xdata::UnsignedInteger64> i2oRUSendCountBU_;
      xdata::Vector<xdata::UnsignedInteger64> i2oBUCachePayloadBU_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit>
evb::readoutunit::BUproxy<ReadoutUnit>::BUproxy(ReadoutUnit* readoutUnit) :
readoutUnit_(readoutUnit),
configuration_(readoutUnit->getConfiguration()),
tid_(0),
doProcessing_(false),
fragmentRequestFIFO_("fragmentRequestFIFO")
{
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", "sudapl");
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
void evb::readoutunit::BUproxy<ReadoutUnit>::startProcessingWorkLoop()
{
  action_ = toolbox::task::bind(this, &evb::readoutunit::BUproxy<ReadoutUnit>::process,
    readoutUnit_->getIdentifier("process") );
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::startProcessing()
{
  resetMonitoringCounters();

  doProcessing_ = true;

  for (uint32_t i=0; i < configuration_->numberOfResponders; ++i)
  {
    workLoops_.at(i)->submit(action_);
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::drain()
{
  while ( processesActive_.any() ) ::usleep(1000);
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::stopProcessing()
{
  doProcessing_ = false;

  while ( processesActive_.any() ) ::usleep(1000);
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::readoutMsgCallback(toolbox::mem::Reference* bufRef)
{
  msg::ReadoutMsg* readoutMsg =
    (msg::ReadoutMsg*)bufRef->getDataLocation();

  FragmentRequestPtr fragmentRequest( new FragmentRequest );
  fragmentRequest->buTid = readoutMsg->buTid;
  fragmentRequest->buResourceId = readoutMsg->buResourceId;
  fragmentRequest->nbRequests = readoutMsg->nbRequests;
  fillRequest(readoutMsg, fragmentRequest);

  updateRequestCounters(fragmentRequest);

  while ( ! fragmentRequestFIFO_.enq(fragmentRequest) ) ::usleep(1000);

  bufRef->release();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::updateRequestCounters(const FragmentRequestPtr fragmentRequest)
{
  boost::mutex::scoped_lock sl(requestMonitoringMutex_);

  requestMonitoring_.payload +=
    sizeof(msg::ReadoutMsg) +
    (fragmentRequest->ruTids.size()|0x1) * sizeof(I2O_TID); // odd number of I2O_TIDs to align header to 64-bits
  if ( !fragmentRequest->evbIds.empty() )
    requestMonitoring_.payload += fragmentRequest->nbRequests * sizeof(EvBid);
  requestMonitoring_.logicalCount += fragmentRequest->nbRequests;
  ++requestMonitoring_.i2oCount;
  requestMonitoring_.logicalCountPerBU[fragmentRequest->buTid] += fragmentRequest->nbRequests;
}


template<class ReadoutUnit>
bool evb::readoutunit::BUproxy<ReadoutUnit>::process(toolbox::task::WorkLoop* wl)
{
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
    + ((nbRUtids+1)&~1) * sizeof(I2O_TID); // always have an even number of 32-bit I2O_TIDs to keep 64-bit alignement

  assert( blockHeaderSize % 8 == 0 );
  assert( blockHeaderSize < configuration_->blockSize );
  unsigned char* payload = (unsigned char*)head->getDataLocation() + blockHeaderSize;
  uint32_t remainingPayloadSize = configuration_->blockSize - blockHeaderSize;
  assert( remainingPayloadSize > sizeof(msg::SuperFragment) );

  for (uint32_t i=0; i < nbSuperFragments; ++i)
  {
    const FragmentChainPtr superFragment = superFragments[i];
    const uint32_t superFragmentSize = superFragment->getSize();
    uint32_t remainingSuperFragmentSize = superFragmentSize;

    fillSuperFragmentHeader(payload,remainingPayloadSize,i+1,superFragmentSize,remainingSuperFragmentSize);

    toolbox::mem::Reference* currentFragment = superFragment->head();

    while ( currentFragment )
    {
      ferolh_t* ferolHeader;
      uint32_t ferolOffset = sizeof(I2O_DATA_READY_MESSAGE_FRAME);

      do
      {
        if ( ferolOffset > (currentFragment->getDataSize()) )
        {
          currentFragment = currentFragment->getNextReference();
          ferolOffset = sizeof(I2O_DATA_READY_MESSAGE_FRAME);

          if (currentFragment == 0)
          {
            XCEPT_RAISE(exception::DataCorruption, "The FEROL data overruns the end of the fragment buffer");
          }
        }

        const unsigned char* ferolData = (unsigned char*)currentFragment->getDataLocation()
          + ferolOffset;

        ferolHeader = (ferolh_t*)ferolData;
        assert( ferolHeader->signature() == FEROL_SIGNATURE );

        if ( ferolHeader->is_first_packet() )
        {
          const fedh_t* fedHeader = (fedh_t*)(ferolData + sizeof(ferolh_t));
          assert( FED_HCTRLID_EXTRACT(fedHeader->eventid) == FED_SLINK_START_MARKER );
        }

        uint32_t currentFragmentSize = ferolHeader->data_length();
        ferolOffset += currentFragmentSize + sizeof(ferolh_t);
        uint32_t copiedSize = sizeof(ferolh_t); // skip the ferol header

        while ( currentFragmentSize > remainingPayloadSize )
        {
          // fill the remaining block
          memcpy(payload, ferolData + copiedSize, remainingPayloadSize);
          copiedSize += remainingPayloadSize;
          currentFragmentSize -= remainingPayloadSize;
          remainingSuperFragmentSize -= remainingPayloadSize;

          // get a new block
          toolbox::mem::Reference* nextBlock = getNextBlock(++blockNb);
          payload = (unsigned char*)nextBlock->getDataLocation() + blockHeaderSize;
          remainingPayloadSize = configuration_->blockSize - blockHeaderSize;
          fillSuperFragmentHeader(payload,remainingPayloadSize,i+1,superFragmentSize,remainingSuperFragmentSize);
          tail->setNextReference(nextBlock);
          tail = nextBlock;
        }

        // fill the remaining fragment into the block
        memcpy(payload, ferolData + copiedSize, currentFragmentSize);
        payload += currentFragmentSize;
        remainingPayloadSize -= currentFragmentSize;
        remainingSuperFragmentSize -= currentFragmentSize;
      }
      while ( ! ferolHeader->is_last_packet() );

      const fedt_t* trailer = (fedt_t*)(payload - sizeof(fedt_t));
      assert ( FED_TCTRLID_EXTRACT(trailer->eventsize) == FED_SLINK_END_MARKER );

      currentFragment = currentFragment->getNextReference();
    }
  }

  xdaq::ApplicationDescriptor *bu = 0;
  try
  {
    bu = i2o::utils::getAddressMap()->getApplicationDescriptor(fragmentRequest->buTid);
  }
  catch(xcept::Exception& e)
  {
    std::ostringstream oss;
    oss << "Failed to get application descriptor for BU with tid ";
    oss << fragmentRequest->buTid;
    XCEPT_RAISE(exception::I2O, oss.str());
  }

  toolbox::mem::Reference* bufRef = head;
  uint32_t payloadSize = 0;
  uint32_t i2oCount = 0;
  uint32_t lastEventNumberToBUs = 0;

  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

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

    unsigned char* payload = (unsigned char*)&dataBlockMsg->evbIds[0];
    for (uint32_t i=0; i < nbSuperFragments; ++i)
    {
      memcpy(payload,&fragmentRequest->evbIds[i],sizeof(EvBid));
      payload += sizeof(EvBid);
      lastEventNumberToBUs = fragmentRequest->evbIds[i].eventNumber();
    }
    for (uint32_t i=0; i < nbRUtids; ++i)
    {
      memcpy(payload,&fragmentRequest->ruTids[i],sizeof(I2O_TID));
      payload += sizeof(I2O_TID);
    }

    payloadSize += (stdMsg->MessageSize << 2) - blockHeaderSize;
    ++i2oCount;

    try
    {
      readoutUnit_->getApplicationContext()->
        postFrame(
          bufRef,
          readoutUnit_->getApplicationDescriptor(),
          bu//,
          //i2oExceptionHandler_,
          //bu
        );
    }
    catch(xcept::Exception& e)
    {
      std::ostringstream oss;
      oss << "Failed to send super fragment to BU TID ";
      oss << fragmentRequest->buTid;
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }

    bufRef = nextRef;
  }

  dataMonitoring_.lastEventNumberToBUs = lastEventNumberToBUs;
  dataMonitoring_.i2oCount += i2oCount;
  dataMonitoring_.payload += payloadSize;
  dataMonitoring_.logicalCount += nbSuperFragments;
  dataMonitoring_.payloadPerBU[fragmentRequest->buTid] += payloadSize;
}


template<class ReadoutUnit>
toolbox::mem::Reference* evb::readoutunit::BUproxy<ReadoutUnit>::getNextBlock
(
  const uint32_t blockNb
) const
{
  toolbox::mem::Reference* bufRef =
    toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,configuration_->blockSize);
  bufRef->setDataSize(configuration_->blockSize);

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
  const uint32_t superFragmentSize,
  const uint32_t currentFragmentSize
) const
{
  if ( remainingPayloadSize < sizeof(msg::SuperFragment) )
  {
    remainingPayloadSize = 0;
    return;
  }

  msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;

  payload += sizeof(msg::SuperFragment);
  remainingPayloadSize -= sizeof(msg::SuperFragment);

  superFragmentMsg->superFragmentNb = superFragmentNb;
  superFragmentMsg->totalSize = superFragmentSize;

  superFragmentMsg->partSize = currentFragmentSize > remainingPayloadSize ? remainingPayloadSize : currentFragmentSize;
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::configure()
{
  fragmentRequestFIFO_.clear();
  fragmentRequestFIFO_.resize(configuration_->fragmentRequestFIFOCapacity);

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

  resetMonitoringCounters();
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::createProcessingWorkLoops()
{
  processesActive_.clear();
  processesActive_.resize(configuration_->numberOfResponders);

  const std::string identifier = readoutUnit_->getIdentifier();

  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=workLoops_.size(); i < configuration_->numberOfResponders; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "Responder_" << i;
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
  lastEventNumberToBUs_ = 0;
  i2oBUCacheCount_ = 0;
  i2oRUSendCountBU_.clear();
  i2oBUCachePayloadBU_.clear();

  items.add("lastEventNumberToBUs", &lastEventNumberToBUs_);
  items.add("i2oBUCacheCount", &i2oBUCacheCount_);
  items.add("i2oRUSendCountBU", &i2oRUSendCountBU_);
  items.add("i2oBUCachePayloadBU", &i2oBUCachePayloadBU_);
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);

    i2oRUSendCountBU_.clear();
    i2oRUSendCountBU_.reserve(requestMonitoring_.logicalCountPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = requestMonitoring_.logicalCountPerBU.begin(),
           itEnd = requestMonitoring_.logicalCountPerBU.end();
         it != itEnd; ++it)
    {
      i2oRUSendCountBU_.push_back(it->second);
    }
  }
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);

    lastEventNumberToBUs_ = dataMonitoring_.lastEventNumberToBUs;
    i2oBUCacheCount_ = dataMonitoring_.logicalCount;

    i2oBUCachePayloadBU_.clear();
    i2oBUCachePayloadBU_.reserve(dataMonitoring_.payloadPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = dataMonitoring_.payloadPerBU.begin(),
           itEnd = dataMonitoring_.payloadPerBU.end();
         it != itEnd; ++it)
    {
      i2oBUCachePayloadBU_.push_back(it->second);
    }
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::resetMonitoringCounters()
{

  {
    boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
    requestMonitoring_.payload = 0;
    requestMonitoring_.logicalCount = 0;
    requestMonitoring_.i2oCount = 0;
    requestMonitoring_.logicalCountPerBU.clear();
  }
  {
    boost::mutex::scoped_lock dsl(dataMonitoringMutex_);
    dataMonitoring_.lastEventNumberToBUs = 0;
    dataMonitoring_.payload = 0;
    dataMonitoring_.logicalCount = 0;
    dataMonitoring_.i2oCount = 0;
    dataMonitoring_.payloadPerBU.clear();
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUproxy<ReadoutUnit>::printHtml(xgi::Output *out) const
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;

  const std::_Ios_Fmtflags originalFlags=out->flags();
  const int originalPrecision=out->precision();
  out->setf(std::ios::fixed);
  out->precision(0);

  boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
  boost::mutex::scoped_lock dsl(dataMonitoringMutex_);

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last evt number to BUs</td>"                       << std::endl;
  *out << "<td>" << dataMonitoring_.lastEventNumberToBUs << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">BU requests</th>"                    << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>payload (kB)</td>"                                 << std::endl;
  *out << "<td>" << requestMonitoring_.payload / 1e3 << "</td>"   << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>logical count</td>"                                << std::endl;
  *out << "<td>" << requestMonitoring_.logicalCount << "</td>"    << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>I2O count</td>"                                    << std::endl;
  *out << "<td>" << requestMonitoring_.i2oCount << "</td>"        << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">BU events cache</th>"                << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>payload (MB)</td>"                                 << std::endl;
  *out << "<td>" << dataMonitoring_.payload / 1e6 << "</td>"      << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>logical count</td>"                                << std::endl;
  *out << "<td>" << dataMonitoring_.logicalCount << "</td>"       << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>I2O count</td>"                                    << std::endl;
  *out << "<td>" << dataMonitoring_.i2oCount << "</td>"           << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  fragmentRequestFIFO_.printHtml(out, readoutUnit_->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Statistics per BU</th>"              << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>Instance</td>"                                     << std::endl;
  *out << "<td>Nb requests</td>"                                  << std::endl;
  *out << "<td>Data payload (MB)</td>"                            << std::endl;
  *out << "</tr>"                                                 << std::endl;

  CountsPerBU::const_iterator it, itEnd;
  for (it=requestMonitoring_.logicalCountPerBU.begin(), itEnd = requestMonitoring_.logicalCountPerBU.end();
       it != itEnd; ++it)
  {
    const uint32_t buTID = it->first;

    *out << "<tr>"                                                << std::endl;
    *out << "<td>BU_" << buTID << "</td>"                         << std::endl;
    *out << "<td>" << requestMonitoring_.logicalCountPerBU.at(buTID) << "</td>" << std::endl;
    *out << "<td>";
    try
    {
      *out << dataMonitoring_.payloadPerBU.at(buTID) / 1e6 << "</td>" << std::endl;
    }
    catch(std::out_of_range)
    {
      *out << "0</td>"                                            << std::endl;
    }
    *out << "</tr>"                                               << std::endl;
  }
  *out << "</table>"                                              << std::endl;

  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  out->flags(originalFlags);
  out->precision(originalPrecision);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}

#endif // _evb_readoutunit_BUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
