#include "i2o/Method.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/bu/RUproxy.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


evb::bu::RUproxy::RUproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
RUbroadcaster(app,fastCtrlMsgPool),
fragmentFIFO_("fragmentFIFO")
{
  resetMonitoringCounters();
}


void evb::bu::RUproxy::superFragmentCallback(toolbox::mem::Reference* bufRef)
{
  while (bufRef)
  {
    updateFragmentCounters(bufRef);
    while ( ! fragmentFIFO_.enq(bufRef) ) { ::usleep(1000); }
    bufRef = bufRef->getNextReference();
  }
}


void evb::bu::RUproxy::updateFragmentCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
  
  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* superFragmentMsg =
    (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const size_t payload =
    (stdMsg->MessageSize << 2) - sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
  const uint32_t ruTid = stdMsg->InitiatorAddress;
  
  fragmentMonitoring_.payload += payload;
  fragmentMonitoring_.payloadPerRU[ruTid] += payload;
  ++fragmentMonitoring_.i2oCount;
  if ( superFragmentMsg->blockNb == superFragmentMsg->nbBlocks )
  {
    ++fragmentMonitoring_.logicalCount;
    ++fragmentMonitoring_.logicalCountPerRU[ruTid];
  }
}


bool evb::bu::RUproxy::getData
(
  toolbox::mem::Reference*& bufRef
)
{
  return fragmentFIFO_.deq(bufRef);
}


void evb::bu::RUproxy::requestTriggerData
(
  const uint32_t buResourceId,
  const uint32_t count
)
{
  toolbox::mem::Reference* rqstBufRef = 
    toolbox::mem::getMemoryPoolFactory()->
    getFrame(fastCtrlMsgPool_, sizeof(msg::RqstForFragmentsMsg));
  
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  
  msg::RqstForFragmentsMsg* rqstMsg = (msg::RqstForFragmentsMsg*)stdMsg;
  rqstMsg->buResourceId = buResourceId;
  rqstMsg->nbRequests = -1 * count; // not specifying any EvbIds

  sendToAllRUs(rqstBufRef, sizeof(msg::RqstForFragmentsMsg));

  // Free the requests message (its copies were sent not it)
  rqstBufRef->release();

  boost::mutex::scoped_lock sl(triggerRequestMonitoringMutex_);
  
  triggerRequestMonitoring_.payload += sizeof(msg::RqstForFragmentsMsg);
  triggerRequestMonitoring_.logicalCount += count;
  ++triggerRequestMonitoring_.i2oCount;
}


void evb::bu::RUproxy::requestFragments
(
  const uint32_t buResourceId,
  const std::vector<EvBid>& evbIds
)
{
  const size_t requestsCount = evbIds.size();
  const size_t msgSize = sizeof(msg::RqstForFragmentsMsg) +
    (requestsCount - 1) * sizeof(EvBid);

  toolbox::mem::Reference* rqstBufRef =
    toolbox::mem::getMemoryPoolFactory()->
    getFrame(fastCtrlMsgPool_, msgSize);
  
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)rqstBufRef->getDataLocation();
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  
  msg::RqstForFragmentsMsg* rqstMsg = (msg::RqstForFragmentsMsg*)stdMsg;
  rqstMsg->buResourceId = buResourceId;
  rqstMsg->nbRequests = requestsCount;

  for (size_t i=0; i < requestsCount; ++i)
    rqstMsg->evbIds[i] = evbIds[i];

  sendToAllRUs(rqstBufRef, msgSize);

  // Free the requests message (its copies were sent not it)
  rqstBufRef->release();

  boost::mutex::scoped_lock sl(fragmentRequestMonitoringMutex_);
  
  fragmentRequestMonitoring_.payload += msgSize;
  fragmentRequestMonitoring_.logicalCount += requestsCount;
  fragmentRequestMonitoring_.i2oCount += participatingRUs_.size();
}


void evb::bu::RUproxy::appendConfigurationItems(InfoSpaceItems& params)
{
  fragmentFIFOCapacity_ = 16384;
  
  ruParams_.clear();
  ruParams_.add("fragmentFIFOCapacity", &fragmentFIFOCapacity_);
  
  initRuInstances(ruParams_);
  
  params.add(ruParams_);
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
}


void evb::bu::RUproxy::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( fragmentFIFO_.deq(bufRef) ) { bufRef->release(); }
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
  
  ruParams_.printHtml("Configuration", out);

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
    *out << "<td>Nb fragments</td>"                                 << std::endl;
    *out << "<td>Data payload (MB)</td>"                            << std::endl;
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
