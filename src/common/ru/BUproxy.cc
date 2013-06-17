#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "interface/shared/i2ogevb2g.h"
#include "evb/RU.h"
#include "evb/ru/BUproxy.h"
#include "evb/Exception.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <cmath>
#include <string.h>


evb::ru::BUproxy::BUproxy
(
  RU* ru,
  boost::shared_ptr<ru::Input> input
) :
ru_(ru),
input_(input),
tid_(0),
doProcessing_(false),
processActive_(false),
requestFIFO_("requestFIFO")
{
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", "udapl");
    superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    toolbox::net::URN poolURN("toolbox-mem-pool",ru_->getApplicationDescriptor()->getURN());
    try
    {
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        createPool(poolURN, new toolbox::mem::HeapAllocator());
    }
    catch (toolbox::mem::exception::DuplicateMemoryPool)
    {
      // Pool already exists from a previous construction of this class
      // Note that destroying the pool in the destructor is not working
      // because the previous instance is destroyed after the new one
      // is constructed.
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        findPool(poolURN);
    }
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for super-fragments.", e);
  }

  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::ru::BUproxy::startProcessingWorkLoop()
{
  try
  {
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( ru_->getIdentifier("RequestProcessing"), "waiting" );
    
    if ( ! processingWL_->isActive() ) processingWL_->activate();

    fragmentAction_ =
        toolbox::task::bind(this, &evb::ru::BUproxy::processSuperFragments,
          ru_->getIdentifier("processSuperFragments") );
    
    triggerAction_ =
        toolbox::task::bind(this, &evb::ru::BUproxy::processTriggers,
          ru_->getIdentifier("processTriggers") );
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void evb::ru::BUproxy::startProcessing()
{
  doProcessing_ = true;
  
  if ( hasTriggerFED_ )
    processingWL_->submit(triggerAction_);
  else
    processingWL_->submit(fragmentAction_);
}


void evb::ru::BUproxy::stopProcessing()
{
  doProcessing_ = false;

  while ( processActive_ ) ::usleep(1000);
}


void evb::ru::BUproxy::rqstForFragmentsMsgCallback(toolbox::mem::Reference* bufRef)
{
  msg::RqstForFragmentsMsg* rqstMsg =
    (msg::RqstForFragmentsMsg*)bufRef->getDataLocation();
  
  Request request;
  request.buTid = ((I2O_MESSAGE_FRAME*)rqstMsg)->InitiatorAddress;
  request.buResourceId = rqstMsg->buResourceId;
  
  // Only react on request if we have the trigger FED and the nbRequests is negative,
  // or if we don't have the trigger FED and specific evbIds are requested
  if ( hasTriggerFED_ )
  {
    if ( rqstMsg->nbRequests < 0 )
    {
      request.nbRequests = abs(rqstMsg->nbRequests);
      updateRequestCounters(request);
      
      while( ! requestFIFO_.enq(request) ) ::usleep(1000);
    }
  }
  else if ( rqstMsg->nbRequests > 0 )
  {
    request.nbRequests = rqstMsg->nbRequests;
    for (int32_t i = 0; i < rqstMsg->nbRequests; ++i)
      request.evbIds.push_back( rqstMsg->evbIds[i] );
    updateRequestCounters(request);

    while( ! requestFIFO_.enq(request) ) ::usleep(1000);
  }

  bufRef->release();
}


void evb::ru::BUproxy::updateRequestCounters(const Request& request)
{
  boost::mutex::scoped_lock sl(requestMonitoringMutex_);
  
  requestMonitoring_.payload += sizeof(msg::RqstForFragmentsMsg);
  if ( !hasTriggerFED_ )
    requestMonitoring_.payload += (request.nbRequests-1) * sizeof(EvBid);
  requestMonitoring_.logicalCount += request.nbRequests;
  ++requestMonitoring_.i2oCount;
  requestMonitoring_.logicalCountPerBU[request.buTid] += request.nbRequests;
}


bool evb::ru::BUproxy::processSuperFragments(toolbox::task::WorkLoop* wl)
{
  processActive_ = true;

  Request request;
  SuperFragments superFragments;
  
  while ( doProcessing_ && requestFIFO_.deq(request) )
  {
    try
    {
      for (uint32_t i=0; i < request.nbRequests; ++i)
      {
        const EvBid& evbId = request.evbIds.at(i);
        FragmentChainPtr superFragment;
        
        while ( doProcessing_ && !input_->getSuperFragmentWithEvBid(evbId, superFragment) ) ::usleep(1000);

        if ( superFragment->isValid() ) superFragments.push_back(superFragment);
      }
      
      sendData(request, superFragments);
    }
    catch(xcept::Exception &e)
    {
      processActive_ = false;
      stateMachine_->processFSMEvent( Fail(e) );
    }
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


bool evb::ru::BUproxy::processTriggers(toolbox::task::WorkLoop* wl)
{
  processActive_ = true;

  Request request;
  while ( doProcessing_ && requestFIFO_.deq(request) )
  {
    try
    {
      request.evbIds.clear();
      request.evbIds.reserve(request.nbRequests);
      
      uint32_t tries = 0;
      SuperFragments superFragments;
      FragmentChainPtr superFragment;
      
      while ( doProcessing_ && !input_->getNextAvailableSuperFragment(superFragment) ) ::usleep(1000);
      
      if ( superFragment->isValid() )
      {
        superFragments.push_back(superFragment);
        request.evbIds.push_back( superFragment->getEvBid() );
      }
      
      while ( doProcessing_ && request.evbIds.size() < request.nbRequests && tries < maxTriggerAgeMSec_ )
      {
        if ( input_->getNextAvailableSuperFragment(superFragment) )
        {
           superFragments.push_back(superFragment);
           request.evbIds.push_back( superFragment->getEvBid() );
        }
        else
        {
          ::usleep(1000);
        }
        ++tries;
      }
      
      request.nbRequests = request.evbIds.size();
      sendData(request, superFragments);
    }
    catch(xcept::Exception &e)
    {
      processActive_ = false;
      stateMachine_->processFSMEvent( Fail(e) );
    }
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


void evb::ru::BUproxy::sendData
(
  const Request& request,
  const SuperFragments& superFragments
)
{
  uint32_t blockNb = 1;
  uint32_t superFragmentNb = 1;
  const uint32_t nbSuperFragments = superFragments.size();
  SuperFragments::const_iterator superFragmentIter = superFragments.begin();

  toolbox::mem::Reference* head = getNextBlock(blockNb);
  toolbox::mem::Reference* tail = head;
  
  const size_t headerSize =
    sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME)
    + (nbSuperFragments-1) * sizeof(EvBid);
  assert( headerSize < blockSize_ );
  unsigned char* payload = (unsigned char*)head->getDataLocation() + headerSize;
  size_t remainingPayloadSize = blockSize_ - headerSize;

  do
  {
    if ( remainingPayloadSize > sizeof(msg::SuperFragment) )
      fillSuperFragmentHeader(payload,remainingPayloadSize,superFragmentNb,(*superFragmentIter)->getSize());
    else
      remainingPayloadSize = 0;
    
    toolbox::mem::Reference* currentFragment = (*superFragmentIter)->head();
    
    while (currentFragment)
    {
      size_t currentFragmentSize =
        ((I2O_DATA_READY_MESSAGE_FRAME*)currentFragment->getDataLocation())->totalLength;
      size_t copiedSize = 0;
      
      while ( currentFragmentSize > remainingPayloadSize )
      {
        // fill the remaining block
        memcpy(payload, (char*)currentFragment->getDataLocation() + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + copiedSize,
          remainingPayloadSize);
        copiedSize += remainingPayloadSize;
        currentFragmentSize -= remainingPayloadSize;
        
        // get a new block
        toolbox::mem::Reference* nextBlock = getNextBlock(++blockNb);
        payload = (unsigned char*)nextBlock->getDataLocation() + headerSize;
        remainingPayloadSize = blockSize_ - headerSize;
        fillSuperFragmentHeader(payload,remainingPayloadSize,superFragmentNb,(*superFragmentIter)->getSize());
        tail->setNextReference(nextBlock);
        tail = nextBlock;
      }
      
      // fill the remaining fragment into the block
      memcpy(payload, (char*)currentFragment->getDataLocation() + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + copiedSize,
        currentFragmentSize);
      payload += currentFragmentSize;
      remainingPayloadSize -= currentFragmentSize;
      
      currentFragment = currentFragment->getNextReference();
    }
    
  } while ( ++superFragmentIter != superFragments.end() );


  toolbox::mem::Reference* bufRef = head;
  uint32_t payloadSize = 0;
  uint32_t i2oCount = 0;
  uint32_t lastEventNumberToBUs = 0;

  // Prepare each event data block for the BU
  while (bufRef)
  {
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg = (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;

    stdMsg->VersionOffset          = 0;
    stdMsg->MsgFlags               = 0;
    stdMsg->MessageSize            = bufRef->getDataSize() >> 2;
    stdMsg->InitiatorAddress       = tid_;
    stdMsg->TargetAddress          = request.buTid;
    stdMsg->Function               = I2O_PRIVATE_MESSAGE;
    pvtMsg->OrganizationID         = XDAQ_ORGANIZATION_ID;
    pvtMsg->XFunctionCode          = I2O_BU_CACHE;
    dataBlockMsg->buResourceId     = request.buResourceId;
    dataBlockMsg->nbBlocks         = blockNb;
    dataBlockMsg->nbSuperFragments = request.evbIds.size();
    for (uint32_t i=0; i < dataBlockMsg->nbSuperFragments; ++i)
    {
      dataBlockMsg->evbIds[i] = request.evbIds[i];
      if ( lastEventNumberToBUs < request.evbIds[i].eventNumber() )
        lastEventNumberToBUs = request.evbIds[i].eventNumber();
    }
    
    payloadSize += (stdMsg->MessageSize << 2) - headerSize;
    ++i2oCount;

    bufRef = bufRef->getNextReference();
  }

  {
     boost::mutex::scoped_lock sl(dataMonitoringMutex_);

     if ( dataMonitoring_.lastEventNumberToBUs < lastEventNumberToBUs )
       dataMonitoring_.lastEventNumberToBUs = lastEventNumberToBUs;
     dataMonitoring_.i2oCount += i2oCount;
     dataMonitoring_.payload += payloadSize;
     ++dataMonitoring_.logicalCount;
     dataMonitoring_.payloadPerBU[request.buTid] += payloadSize;
  }
  
  xdaq::ApplicationDescriptor *bu = 0;
  try
  {
    bu = i2o::utils::getAddressMap()->getApplicationDescriptor(request.buTid);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor for BU with tid ";
    oss << request.buTid;
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  
  try
  {
    ru_->getApplicationContext()->
      postFrame(
        head,
        ru_->getApplicationDescriptor(),
        bu//,
        //i2oExceptionHandler_,
        //bu
      );
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to send super fragment to BU";
    oss << bu->getInstance();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }
}


toolbox::mem::Reference* evb::ru::BUproxy::getNextBlock
(
  const uint32_t blockNb
)
{
  toolbox::mem::Reference* bufRef =
    toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,blockSize_);
  bufRef->setDataSize(blockSize_);
  
  msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg = (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  dataBlockMsg->blockNb = blockNb;
  
  return bufRef;
}


void evb::ru::BUproxy::fillSuperFragmentHeader
(
  unsigned char*& payload,
  size_t& remainingPayloadSize,
  const uint32_t superFragmentNb,
  const uint32_t superFragmentSize
) const
{
  msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;
  
  superFragmentMsg->superFragmentNb = superFragmentNb;
  superFragmentMsg->totalSize = superFragmentSize;
  superFragmentMsg->partSize = superFragmentSize > remainingPayloadSize ? remainingPayloadSize : superFragmentSize;
  
  payload += sizeof(msg::SuperFragment);
  remainingPayloadSize -= sizeof(msg::SuperFragment);
}


void evb::ru::BUproxy::configure()
{
  clear();

  requestFIFO_.resize(requestFIFOCapacity_);

  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(ru_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }
}


void evb::ru::BUproxy::appendConfigurationItems(InfoSpaceItems& params)
{
  maxTriggerAgeMSec_ = 1000;
  blockSize_ = 4096;
  requestFIFOCapacity_ = 65536;
  hasTriggerFED_ = false;

  buParams_.clear();
  buParams_.add("maxTriggerAgeMSec", &maxTriggerAgeMSec_);
  buParams_.add("blockSize", &blockSize_);
  buParams_.add("requestFIFOCapacity", &requestFIFOCapacity_);
  buParams_.add("hasTriggerFED", &hasTriggerFED_);

  params.add(buParams_);
}


void evb::ru::BUproxy::appendMonitoringItems(InfoSpaceItems& items)
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


void evb::ru::BUproxy::updateMonitoringItems()
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


void evb::ru::BUproxy::resetMonitoringCounters()
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


void evb::ru::BUproxy::clear()
{
  Request request;
  while ( requestFIFO_.deq(request) ) {};
}


void evb::ru::BUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
  boost::mutex::scoped_lock dsl(dataMonitoringMutex_);

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last evt number to BUs</td>"                       << std::endl;
  *out << "<td>" << dataMonitoring_.lastEventNumberToBUs << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">BU requests</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>payload (kB)</td>"                                 << std::endl;
  *out << "<td>" << requestMonitoring_.payload / 0x400 << "</td>" << std::endl;
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
  *out << "<td colspan=\"2\" style=\"text-align:center\">BU events cache</td>" << std::endl;
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
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  requestFIFO_.printHtml(out, ru_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  buParams_.printHtml("Configuration", out);
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">Statistics per BU</td>" << std::endl;
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
    const uint32_t buInstance = it->first;

    *out << "<tr>"                                                << std::endl;
    *out << "<td>BU_" << buInstance << "</td>"                    << std::endl;
    *out << "<td>" << requestMonitoring_.logicalCountPerBU[buInstance] << "</td>" << std::endl;
    *out << "<td>" << dataMonitoring_.payloadPerBU[buInstance] / 0x100000 << "</td>" << std::endl;
    *out << "</tr>"                                               << std::endl;
  }
  *out << "</table>"                                              << std::endl;
  
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


std::ostream& operator<<
(
  std::ostream& str,
  const evb::ru::BUproxy::Request request
)
{
  str << "Request:" << std::endl;
  
  str << "buTid=" << request.buTid << std::endl;
  str << "buResourceId=" << request.buResourceId << std::endl;
  str << "nbRequests=" << request.nbRequests << std::endl;
  if ( !request.evbIds.empty() )
  {
    str << "evbIds:" << std::endl;
    for (uint32_t i=0; i < request.nbRequests; ++i)
      str << "   [" << i << "]: " << request.evbIds[i] << std::endl;
  }
  
  return str;
}

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
