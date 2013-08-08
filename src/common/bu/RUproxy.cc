#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/EventBuilder.h"
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
  boost::shared_ptr<EventBuilder> eventBuilder,
  boost::shared_ptr<ResourceManager> resourceManager,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
bu_(bu),
eventBuilder_(eventBuilder),
resourceManager_(resourceManager),
fastCtrlMsgPool_(fastCtrlMsgPool),
configuration_(bu->getConfiguration()),
doProcessing_(false),
requestFragmentsActive_(false),
acceptI2Omessages_(false),
tid_(0)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::bu::RUproxy::superFragmentCallback(toolbox::mem::Reference* bufRef)
{
  if ( acceptI2Omessages_ )
  {
    try
    {
      while (bufRef)
      {
        const I2O_MESSAGE_FRAME* stdMsg =
          (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
        const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
          (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
        const uint32_t payload = (stdMsg->MessageSize << 2) -
          dataBlockMsg->getHeaderSize();

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
            std::ostringstream oss;
            oss << "Received a first super-fragment block from RU tid " << index.ruTid;
            oss << " for BU resource id " << index.buResourceId;
            oss << " which is already block number " <<  dataBlockMsg->blockNb;
            oss << " of " << dataBlockMsg->nbBlocks;
            XCEPT_RAISE(exception::SuperFragment, oss.str());
          }

          FragmentChainPtr dataBlock( new FragmentChain(dataBlockMsg->nbBlocks) );
          dataBlockPos = dataBlockMap_.insert(dataBlockPos, DataBlockMap::value_type(index,dataBlock));
        }

        if ( ! dataBlockPos->second->append(dataBlockMsg->blockNb,bufRef) )
        {
          std::ostringstream oss;
          oss << "Received a super-fragment block from RU tid " << index.ruTid;
          oss << " for BU resource id " << index.buResourceId;
          oss << " with a duplicated block number " <<  dataBlockMsg->blockNb;
          XCEPT_RAISE(exception::SuperFragment, oss.str());
        }

        resourceManager_->underConstruction(dataBlockMsg);

        if ( dataBlockPos->second->isComplete() )
        {
          eventBuilder_->addSuperFragment(dataBlockMsg->buResourceId,dataBlockPos->second);
          dataBlockMap_.erase(dataBlockPos);
        }

        bufRef = bufRef->getNextReference();
      }
    }
    catch(xcept::Exception& e)
    {
      stateMachine_->processFSMEvent( Fail(e) );
    }
  }
}


void evb::bu::RUproxy::startProcessing()
{
  doProcessing_ = true;
  requestFragmentsWL_->submit(requestFragmentsAction_);
  acceptI2Omessages_ = true;
}


void evb::bu::RUproxy::stopProcessing()
{
  acceptI2Omessages_ = false;
  doProcessing_ = false;
  while (requestFragmentsActive_) ::usleep(1000);

  while ( ! dataBlockMap_.empty() ) ::usleep(1000);
}


void evb::bu::RUproxy::startProcessingWorkLoop()
{
  try
  {
    requestFragmentsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("requestFragments"), "waiting" );

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
    std::string msg = "Failed to start workloop 'requestFragments'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::RUproxy::requestFragments(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  requestFragmentsActive_ = true;

  try
  {
    uint32_t buResourceId;
    const uint32_t msgSize = sizeof(msg::ReadoutMsg)+sizeof(I2O_TID);

    while ( resourceManager_->getResourceId(buResourceId) )
    {
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
      stdMsg->TargetAddress    = evm_.tid;
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      readoutMsg->buTid        = tid_;
      readoutMsg->buResourceId = buResourceId;
      readoutMsg->nbRequests   = configuration_->eventsPerRequest;
      readoutMsg->nbRUtids     = 0; // will be filled by EVM

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
      catch(xcept::Exception& e)
      {
        std::ostringstream oss;
        oss << "Failed to send message to EVM TID ";
        oss << evm_.tid;
        XCEPT_RETHROW(exception::I2O, oss.str(), e);
      }

      boost::mutex::scoped_lock sl(requestMonitoringMutex_);

      requestMonitoring_.payload += msgSize;
      requestMonitoring_.logicalCount += configuration_->eventsPerRequest;
      ++requestMonitoring_.i2oCount;
    }
  }
  catch(xcept::Exception& e)
  {
    requestFragmentsActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  requestFragmentsActive_ = false;

  return doProcessing_;
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
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    i2oRUSendCount_ = requestMonitoring_.logicalCount;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    i2oBUCacheCount_ = fragmentMonitoring_.logicalCount;
  }
}


void evb::bu::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    requestMonitoring_.payload = 0;
    requestMonitoring_.logicalCount = 0;
    requestMonitoring_.i2oCount = 0;
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
  acceptI2Omessages_ = false;
  clear();

  getApplicationDescriptors();
}


void evb::bu::RUproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(bu_->getApplicationDescriptor());
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::I2O,
      "Failed to get I2O TID for this application.", e);
  }

  getApplicationDescriptorForEVM();
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
    catch(xcept::Exception& e)
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
    catch(xcept::Exception& e)
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
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::I2O,
      "Failed to get the I2O TID of the EVM", e);
  }
}


void evb::bu::RUproxy::clear()
{
  dataBlockMap_.clear();
}


void evb::bu::RUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;

  const std::_Ios_Fmtflags originalFlags=out->flags();
  const int originalPrecision=out->precision();
  out->setf(std::ios::fixed);
  out->precision(0);

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
    *out << "<td>" << fragmentMonitoring_.payload / 1e6 << "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << fragmentMonitoring_.logicalCount << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << fragmentMonitoring_.i2oCount << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<th colspan=\"2\">Requests</th>"                       << std::endl;
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
  }

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
      *out << "<td>" << fragmentMonitoring_.payloadPerRU[it->first] / 1e6 << "</td>" << std::endl;
      *out << "</tr>"                                               << std::endl;
    }
    *out << "</table>"                                              << std::endl;

    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  out->flags(originalFlags);
  out->precision(originalPrecision);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
