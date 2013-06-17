#include "i2o/Method.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/RUproxy.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


evb::bu::RUproxy::RUproxy
(
  BU* bu,
  toolbox::mem::Pool* fastCtrlMsgPool,
  boost::shared_ptr<ResourceManager> resourceManager
) :
RUbroadcaster(bu,fastCtrlMsgPool),
bu_(bu),
resourceManager_(resourceManager),
doProcessing_(false),
requestTriggersActive_(false),
requestFragmentsActive_(false),
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
      
      fragmentMonitoring_.lastEventNumberFromRUs = dataBlockMsg->evbIds[dataBlockMsg->nbSuperFragments-1].eventNumber();
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
        oss << " which is already block " <<  dataBlockMsg->blockNb;
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
      
      I2O_MESSAGE_FRAME* stdMsg =
        (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
      I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
      msg::RqstForFragmentsMsg* rqstMsg = (msg::RqstForFragmentsMsg*)stdMsg;
      
      stdMsg->VersionOffset    = 0;
      stdMsg->MsgFlags         = 0;
      stdMsg->MessageSize      = sizeof(msg::RqstForFragmentsMsg) >> 2;
      stdMsg->InitiatorAddress = tid_;
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      rqstMsg->buResourceId    = buResourceId;
      rqstMsg->nbRequests      = -1 * eventsPerRequest_; // not specifying any EvbIds
      
      sendToAllRUs(rqstBufRef, sizeof(msg::RqstForFragmentsMsg));
      
      // Free the requests message (its copies were sent not it)
      rqstBufRef->release();
      
      boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
      
      triggerRequestMonitoring_.payload += sizeof(msg::RqstForFragmentsMsg);
      triggerRequestMonitoring_.logicalCount += eventsPerRequest_;
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
    while ( doProcessing_ && resourceManager_->getRequest(request) && participatingRUs_.size()>1 )
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
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
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


void evb::bu::RUproxy::appendConfigurationItems(InfoSpaceItems& params)
{
  eventsPerRequest_ = 5;
  fragmentFIFOCapacity_ = 16384;
  
  ruProxyParams_.clear();
  ruProxyParams_.add("eventsPerRequest", &eventsPerRequest_);
  ruProxyParams_.add("fragmentFIFOCapacity", &fragmentFIFOCapacity_);
  
  initRuInstances(ruProxyParams_);
  
  params.add(ruProxyParams_);
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

  fragmentFIFO_.resize(fragmentFIFOCapacity_);

  getApplicationDescriptors();
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
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  {
    boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Requests for trigger data</td>" << std::endl;
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
    *out << "<td colspan=\"2\" style=\"text-align:center\">Requests for fragments</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << fragmentRequestMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Event data</td>" << std::endl;
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
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fragmentFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  ruProxyParams_.printHtml("Configuration", out);

  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Statistics per RU</td>" << std::endl;
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
      *out << "<td>RU_" << it->first << "</td>"                     << std::endl;
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
