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

#include <string.h>


evb::evm::RUproxy::RUproxy
(
  EVM* evm,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
evm_(evm),
fastCtrlMsgPool_(fastCtrlMsgPool),
allocateFIFO_("allocateFIFO"),
doProcessing_(false),
assignEventsActive_(false),
tid_(0)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::evm::RUproxy::sendRequest(const readoutunit::FragmentRequestPtr fragmentRequest)
{
  if ( participatingRUs_.empty() ) return;

  while ( ! allocateFIFO_.enq(fragmentRequest) ) ::usleep(1000);
}


void evb::evm::RUproxy::startProcessing()
{
  doProcessing_ = true;

  if ( ! participatingRUs_.empty() )
    assignEventsWL_->submit(assignEventsAction_);
}


void evb::evm::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while (assignEventsActive_) ::usleep(1000);
}


void evb::evm::RUproxy::startProcessingWorkLoop()
{
  try
  {
    assignEventsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( evm_->getIdentifier("assignEvents"), "waiting" );

    if ( ! assignEventsWL_->isActive() )
    {
      assignEventsAction_ =
        toolbox::task::bind(this, &evb::evm::RUproxy::assignEvents,
          evm_->getIdentifier("ruProxyAssignEvents") );

      assignEventsWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'assignEvents'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::evm::RUproxy::assignEvents(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  assignEventsActive_ = true;

  try
  {
    readoutunit::FragmentRequestPtr fragmentRequest;
    while ( allocateFIFO_.deq(fragmentRequest)  )
    {
      const uint16_t requestsCount = fragmentRequest->nbRequests;
      const uint16_t ruCount = fragmentRequest->ruTids.size();
      const size_t msgSize = sizeof(msg::ReadoutMsg) +
        requestsCount * sizeof(EvBid) +
        (ruCount|0x1) * sizeof(I2O_TID); // odd number of I2O_TIDs to align header to 64-bits

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
      stdMsg->Function         = I2O_PRIVATE_MESSAGE;
      pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
      pvtMsg->XFunctionCode    = I2O_SHIP_FRAGMENTS;
      readoutMsg->buTid        = fragmentRequest->buTid;
      readoutMsg->buResourceId = fragmentRequest->buResourceId;
      readoutMsg->nbRequests   = requestsCount;
      readoutMsg->nbRUtids     = ruCount;

      uint32_t lastEventNumberToRUs = 0;
      unsigned char* payload = (unsigned char*)&readoutMsg->evbIds[0];
      for (uint32_t i = 0; i < requestsCount; ++i)
      {
        memcpy(payload,&fragmentRequest->evbIds[i],sizeof(EvBid));
        payload += sizeof(EvBid);
        lastEventNumberToRUs = fragmentRequest->evbIds[i].eventNumber();
      }

      for (uint32_t i = 0; i < ruCount; ++i)
      {
        memcpy(payload,&fragmentRequest->ruTids[i],sizeof(I2O_TID));
        payload += sizeof(I2O_TID);
      }

      sendToAllRUs(rqstBufRef, msgSize);

      // Free the requests message (its copies were sent not it)
      rqstBufRef->release();

      boost::mutex::scoped_lock sl(allocateMonitoringMutex_);

      allocateMonitoring_.lastEventNumberToRUs = lastEventNumberToRUs;
      allocateMonitoring_.payload += msgSize*participatingRUs_.size();
      allocateMonitoring_.logicalCount += requestsCount;
      allocateMonitoring_.i2oCount += participatingRUs_.size();
    }
  }
  catch(xcept::Exception &e)
  {
    assignEventsActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  assignEventsActive_ = false;

  return doProcessing_;
}


void evb::evm::RUproxy::sendToAllRUs
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
      evm_->getApplicationContext()->
        postFrame(
          copyBufRef,
          evm_->getApplicationDescriptor(),
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


void evb::evm::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
}


void evb::evm::RUproxy::updateMonitoringItems()
{
}


void evb::evm::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    allocateMonitoring_.lastEventNumberToRUs = 0;
    allocateMonitoring_.payload = 0;
    allocateMonitoring_.logicalCount = 0;
    allocateMonitoring_.i2oCount = 0;
  }
}


void evb::evm::RUproxy::configure()
{
  clear();

  allocateFIFO_.resize(evm_->getConfiguration()->fragmentRequestFIFOCapacity);

  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(evm_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }

  // Clear list of participating RUs
  participatingRUs_.clear();
  ruTids_.clear();
  ruTids_.push_back(tid_);

  getApplicationDescriptorsForRUs();
}


void evb::evm::RUproxy::getApplicationDescriptorsForRUs()
{
  std::set<xdaq::ApplicationDescriptor*> ruDescriptors;

  try
  {
    ruDescriptors =
      evm_->getApplicationContext()->
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
    LOG4CPLUS_WARN(evm_->getApplicationLogger(), "There are no RU application descriptors.");

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


void evb::evm::RUproxy::clear()
{
  readoutunit::FragmentRequestPtr fragmentRequest;
  while( allocateFIFO_.deq(fragmentRequest) ) {};
}


void evb::evm::RUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;

  {
    boost::mutex::scoped_lock sl(allocateMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last event number to RUs</td>"                     << std::endl;
    *out << "<td>" << allocateMonitoring_.lastEventNumberToRUs << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << allocateMonitoring_.payload / 1e3 << "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << allocateMonitoring_.logicalCount << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << allocateMonitoring_.i2oCount << "</td>"          << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  allocateFIFO_.printHtml(out, evm_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
