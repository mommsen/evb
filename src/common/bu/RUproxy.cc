#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/RUproxy.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


evb::bu::RUproxy::RUproxy
(
  BU* bu,
  boost::shared_ptr<ResourceManager> resourceManager,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
bu_(bu),
resourceManager_(resourceManager),
fastCtrlMsgPool_(fastCtrlMsgPool),
configuration_(bu->getConfiguration()),
doProcessing_(false),
requestTriggersActive_(false),
requestFragmentsActive_(false),
tid_(0),
fragmentFIFO_("fragmentFIFO")
{
  resetMonitoringCounters();
  startProcessingWorkLoops();
}


void evb::bu::RUproxy::superFragmentCallback(toolbox::mem::Reference* bufRef)
{
  while (bufRef)
  {
    const I2O_MESSAGE_FRAME* stdMsg =
      (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
      (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
    const size_t payload =
      (stdMsg->MessageSize << 2) - sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);

    Index index;
    index.ruTid = stdMsg->InitiatorAddress;
    index.buResourceId = dataBlockMsg->buResourceId;
    
    {
      boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
      
      const uint32_t lastEventNumber = dataBlockMsg->evbIds[dataBlockMsg->nbSuperFragments-1].eventNumber();
      if ( index.ruTid == evm_.tid )
        fragmentMonitoring_.lastEventNumberFromEVM = lastEventNumber;
      else
        fragmentMonitoring_.lastEventNumberFromRUs = lastEventNumber;
      fragmentMonitoring_.payload += payload;
      fragmentMonitoring_.payloadPerRU[index.ruTid] += payload;
      ++fragmentMonitoring_.i2oCount;
      if ( dataBlockMsg->blockNb == dataBlockMsg->nbBlocks )
      {
        ++fragmentMonitoring_.logicalCount;
        ++fragmentMonitoring_.logicalCountPerRU[index.ruTid];
      }
    }
    
    DataBlockMap::iterator dataBlockPos = dataBlockMap_.lower_bound(index);
    if ( dataBlockPos == dataBlockMap_.end() || (dataBlockMap_.key_comp()(index,dataBlockPos->first)) )
    {
      // new data block
      if ( dataBlockMsg->blockNb != 1 )
      {
        std::stringstream oss;
        
        oss << "Received a first super-fragment block from RU tid " << index.ruTid;
        oss << " for BU resource id " << index.buResourceId;
        oss << " which is already block number " <<  dataBlockMsg->blockNb;
        oss << " of " << dataBlockMsg->nbBlocks;
        
        XCEPT_RAISE(exception::EventOrder, oss.str());
      }
      
      FragmentChainPtr dataBlock( new FragmentChain(dataBlockMsg->nbBlocks) );
      dataBlockPos = dataBlockMap_.insert(dataBlockPos, DataBlockMap::value_type(index,dataBlock));
    }
    
    if ( ! dataBlockPos->second->append(dataBlockMsg->blockNb,bufRef) )
    {
      std::stringstream oss;
      
      oss << "Received a super-fragment block from RU tid " << index.ruTid;
      oss << " for BU resource id " << index.buResourceId;
      oss << " with a duplicated block number " <<  dataBlockMsg->blockNb;
      
      XCEPT_RAISE(exception::EventOrder, oss.str());
    }
    
    resourceManager_->underConstruction(dataBlockMsg);
    
    if ( dataBlockPos->second->isComplete() )
    {
      while ( ! fragmentFIFO_.enq(dataBlockPos->second->duplicate()) ) { ::usleep(1000); }
      dataBlockMap_.erase(dataBlockPos);
    }
    
    bufRef = bufRef->getNextReference();
  }
}


bool evb::bu::RUproxy::getData
(
  toolbox::mem::Reference*& bufRef
)
{
  return fragmentFIFO_.deq(bufRef);
}


void evb::bu::RUproxy::startProcessing()
{
  doProcessing_ = true;
  requestTriggersWL_->submit(requestTriggersAction_);
  
  if ( ! participatingRUs_.empty() )
    requestFragmentsWL_->submit(requestFragmentsAction_);
}


void evb::bu::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while (requestTriggersActive_) ::usleep(1000);
  while (requestFragmentsActive_) ::usleep(1000);
}


void evb::bu::RUproxy::startProcessingWorkLoops()
{
  try
  {
    requestTriggersWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("RUproxyRequestTriggers"), "waiting" );
    
    if ( ! requestTriggersWL_->isActive() )
    {
      requestTriggersAction_ =
        toolbox::task::bind(this, &evb::bu::RUproxy::requestTriggers,
          bu_->getIdentifier("ruProxyRequestTriggers") );
    
      requestTriggersWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'RUproxyRequestTriggers'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
  
  try
  {
    requestFragmentsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("RUproxyRequestFragments"), "waiting" );
    
    if ( ! requestFragmentsWL_->isActive() )
    {
      requestFragmentsAction_ =
        toolbox::task::bind(this, &evb::bu::RUproxy::requestFragments,
          bu_->getIdentifier("ruProxyRequestFragments") );
    
      requestFragmentsWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'RUproxyRequestFragments'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::RUproxy::requestTriggers(toolbox::task::WorkLoop*)
{
  ::usleep(1000);
  
  requestTriggersActive_ = true;
  
  try
  {
    uint32_t buResourceId;
    while ( doProcessing_ && resourceManager_->getResourceId(buResourceId) )
    {
      toolbox::mem::Reference* rqstBufRef = 
        toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_, sizeof(msg::RqstForFragmentsMsg));
      rqstBufRef->setDataSize(sizeof(msg::RqstForFragmentsMsg));

      I2O_MESSAGE_FRAME* stdMsg =
        (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
      I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
      msg::RqstForFragmentsMsg* rqstMsg = (msg::RqstForFragmentsMsg*)stdMsg;
      
      stdMsg->VersionOffset    = 0;
      stdMsg->MsgFlags         = 0;
      stdMsg->MessageSize      = sizeof(msg::RqstForFragmentsMsg) >> 2;
      stdMsg->InitiatorAddress = tid_;
      stdMsg->TargetAddress    = evm_.tid;
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      rqstMsg->buResourceId    = buResourceId;
      rqstMsg->nbRequests      = -1 * configuration_->eventsPerRequest; // not specifying any EvbIds

      // Send the request to the EVM      
      try
      {
        bu_->getApplicationContext()->
          postFrame(
            rqstBufRef,
            bu_->getApplicationDescriptor(),
            evm_.descriptor //,
            //i2oExceptionHandler_,
            //it->descriptor
          );
      }
      catch(xcept::Exception &e)
      {
        std::stringstream oss;
        
        oss << "Failed to send message to EVM TID ";
        oss << evm_.tid;
        
        XCEPT_RETHROW(exception::I2O, oss.str(), e);
      }
      
      boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
      
      triggerRequestMonitoring_.payload += sizeof(msg::RqstForFragmentsMsg);
      triggerRequestMonitoring_.logicalCount += configuration_->eventsPerRequest;
      ++triggerRequestMonitoring_.i2oCount;
    }
  }
  catch(xcept::Exception &e)
  {
    requestTriggersActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  requestTriggersActive_ = false;
  
  return doProcessing_;
}


bool evb::bu::RUproxy::requestFragments(toolbox::task::WorkLoop*)
{
  ::usleep(1000);
  
  requestFragmentsActive_ = true;
  
  try
  {
    ResourceManager::RequestPtr request;
    while ( doProcessing_ && resourceManager_->getRequest(request) )
    {
      const size_t requestsCount = request->evbIds.size();
      const size_t msgSize = sizeof(msg::RqstForFragmentsMsg) +
        (requestsCount - 1) * sizeof(EvBid);
      
      toolbox::mem::Reference* rqstBufRef =
        toolbox::mem::getMemoryPoolFactory()->
        getFrame(fastCtrlMsgPool_, msgSize);
      rqstBufRef->setDataSize(msgSize);
      
      I2O_MESSAGE_FRAME* stdMsg =
        (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
      I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
      msg::RqstForFragmentsMsg* rqstMsg = (msg::RqstForFragmentsMsg*)stdMsg;
      
      stdMsg->VersionOffset    = 0;
      stdMsg->MsgFlags         = 0;
      stdMsg->MessageSize      = msgSize >> 2;
      stdMsg->InitiatorAddress = tid_;
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      rqstMsg->buResourceId    = request->buResourceId;
      rqstMsg->nbRequests      = requestsCount;
      
      uint32_t i = 0;
      for (EvBids::const_iterator it = request->evbIds.begin(), itEnd = request->evbIds.end();
           it != itEnd; ++it, ++i)
        rqstMsg->evbIds[i] = *it;
      
      sendToAllRUs(rqstBufRef, msgSize);
      
      // Free the requests message (its copies were sent not it)
      rqstBufRef->release();

      boost::mutex::scoped_lock sl(fragmentRequestMonitoringMutex_);
      
      fragmentRequestMonitoring_.payload += msgSize;
      fragmentRequestMonitoring_.logicalCount += requestsCount;
      fragmentRequestMonitoring_.i2oCount += participatingRUs_.size();
    }
  }
  catch(xcept::Exception &e)
  {
    requestFragmentsActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  requestFragmentsActive_ = false;
  
  return doProcessing_;
}


void evb::bu::RUproxy::sendToAllRUs
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
      bu_->getApplicationContext()->
        postFrame(
          copyBufRef,
          bu_->getApplicationDescriptor(),
          it->descriptor //,
          //i2oExceptionHandler_,
          //it->descriptor
        );
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to send message to RU TID ";
      oss << it->tid;

      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
  }
}


void evb::bu::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
  i2oBUCacheCount_ = 0;
  i2oRUSendCount_ = 0;

  items.add("i2oBUCacheCount", &i2oBUCacheCount_);
  items.add("i2oRUSendCount", &i2oRUSendCount_);
}


void evb::bu::RUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(fragmentRequestMonitoringMutex_);
    i2oRUSendCount_ = fragmentRequestMonitoring_.logicalCount;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    i2oBUCacheCount_ = fragmentMonitoring_.logicalCount;
  }
}


void evb::bu::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
    triggerRequestMonitoring_.payload = 0;
    triggerRequestMonitoring_.logicalCount = 0;
    triggerRequestMonitoring_.i2oCount = 0;
  }
  {
    boost::mutex::scoped_lock sl(fragmentRequestMonitoringMutex_);
    fragmentRequestMonitoring_.payload = 0;
    fragmentRequestMonitoring_.logicalCount = 0;
    fragmentRequestMonitoring_.i2oCount = 0;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    fragmentMonitoring_.lastEventNumberFromEVM = 0;
    fragmentMonitoring_.lastEventNumberFromRUs = 0;
    fragmentMonitoring_.payload = 0;
    fragmentMonitoring_.logicalCount = 0;
    fragmentMonitoring_.i2oCount = 0;
    fragmentMonitoring_.logicalCountPerRU.clear();
    fragmentMonitoring_.payloadPerRU.clear();
  }
}


void evb::bu::RUproxy::configure()
{
  clear();

  fragmentFIFO_.resize(configuration_->fragmentFIFOCapacity.value_);

  getApplicationDescriptors();
}


void evb::bu::RUproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(bu_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }
  
  // Clear list of participating RUs
  participatingRUs_.clear();
  ruTids_.clear();
  
  getApplicationDescriptorForEVM();
  getApplicationDescriptorsForRUs();

  resourceManager_->setHaveRUs( ! participatingRUs_.empty() );
}


void evb::bu::RUproxy::getApplicationDescriptorForEVM()
{
  if (configuration_->evmInstance.value_ < 0)
  {
    // Try to find instance number by assuming the first EVM found is the
    // one to be used.
    
    std::set<xdaq::ApplicationDescriptor*> evmDescriptors;
    
    try
    {
      evmDescriptors =
        bu_->getApplicationContext()->
        getDefaultZone()->
        getApplicationDescriptors("evb::EVM");
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to get EVM application descriptor", e);
    }
    
    if ( evmDescriptors.empty() )
    {
      XCEPT_RAISE(exception::Configuration,
        "Failed to get EVM application descriptor");
    }
    
    evm_.descriptor = *(evmDescriptors.begin());
  }
  else
  {
    try
    {
      evm_.descriptor =
        bu_->getApplicationContext()->
        getDefaultZone()->
        getApplicationDescriptor("evb::EVM",
          configuration_->evmInstance.value_);
    }
    catch(xcept::Exception &e)
    {
      std::ostringstream oss;
      
      oss << "Failed to get application descriptor of EVM";
      oss << configuration_->evmInstance.toString();
      
      XCEPT_RETHROW(exception::Configuration, oss.str(), e);
    }
  }
  
  try
  {
    evm_.tid = i2o::utils::getAddressMap()->getTid(evm_.descriptor);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get the I2O TID of the EVM", e);
  }

  ruTids_.push_back(evm_.tid);
}


void evb::bu::RUproxy::getApplicationDescriptorsForRUs()
{
  std::set<xdaq::ApplicationDescriptor*> ruDescriptors;

  try
  {
    ruDescriptors =
      bu_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("evb::RU");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get RU application descriptor", e);
  }

  if ( ruDescriptors.empty() )
  {
    LOG4CPLUS_WARN(bu_->getApplicationLogger(), "There are no RU application descriptors.");
    
    return;
  }

  std::set<xdaq::ApplicationDescriptor*>::const_iterator it = ruDescriptors.begin();
  const std::set<xdaq::ApplicationDescriptor*>::const_iterator itEnd = ruDescriptors.end();
  for ( ; it != itEnd ; ++it)
  {
    ApplicationDescriptorAndTid ru;
    
    ru.descriptor = *it;
    
    try
    {
      ru.tid = i2o::utils::getAddressMap()->getTid(*it);
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to get I2O TID for RU ";
      oss << (*it)->getInstance();
      
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
    
    if ( ! participatingRUs_.insert(ru).second )
    {
      std::stringstream oss;
      
      oss << "Participating RU instance is a duplicate.";
      oss << " Instance:" << (*it)->getInstance();
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    ruTids_.push_back(ru.tid);
  }
}


void evb::bu::RUproxy::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( fragmentFIFO_.deq(bufRef) ) { bufRef->release(); }
  
  dataBlockMap_.clear();
}


void evb::bu::RUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last event number from EVM</td>"                   << std::endl;
    *out << "<td>" << fragmentMonitoring_.lastEventNumberFromEVM << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last event number from RUs</td>"                   << std::endl;
    *out << "<td>" << fragmentMonitoring_.lastEventNumberFromRUs << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<th colspan=\"2\">Event data</th>"                     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << fragmentMonitoring_.payload / 0x100000 << "</td>"<< std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << fragmentMonitoring_.logicalCount << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << fragmentMonitoring_.i2oCount << "</td>"          << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<th colspan=\"2\">Requests for trigger data</th>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << triggerRequestMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << triggerRequestMonitoring_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << triggerRequestMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(fragmentRequestMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<th colspan=\"2\">Requests for fragments</th>"         << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.logicalCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.i2oCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  fragmentFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<th colspan=\"2\">Statistics per RU</th>"              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\">"                                    << std::endl;
    *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Instance</td>"                                     << std::endl;
    *out << "<td>Fragments</td>"                                    << std::endl;
    *out << "<td>Payload (MB)</td>"                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;

    CountsPerRU::const_iterator it, itEnd;
    for (it=fragmentMonitoring_.logicalCountPerRU.begin(),
           itEnd = fragmentMonitoring_.logicalCountPerRU.end();
         it != itEnd; ++it)
    {
      *out << "<tr>"                                                << std::endl;
      *out << "<td>";
      if ( it->first == evm_.tid )
        *out << "EVM";
      else
        *out << "RU_" << it->first;
      *out << "</td>"                                               << std::endl;
      *out << "<td>" << it->second << "</td>"                       << std::endl;
      *out << "<td>" << fragmentMonitoring_.payloadPerRU[it->first] / 0x100000 << "</td>" << std::endl;
      *out << "</tr>"                                               << std::endl;
    }
    *out << "</table>"                                              << std::endl;

    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
