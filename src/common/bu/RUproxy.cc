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

#include <boost/lexical_cast.hpp>

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
  tid_(0)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();

  curl_ = curl_easy_init();
  if ( ! curl_ )
  {
    XCEPT_RAISE(exception::DiskWriting, "Could not initialize curl for connection to EVM");
  }
  curl_easy_setopt(curl_, CURLOPT_HEADER, 0);
  curl_easy_setopt(curl_, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L); //allow libcurl to follow redirection
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &evb::bu::RUproxy::curlWriter);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &curlBuffer_);
}


evb::bu::RUproxy::~RUproxy()
{
  curl_easy_cleanup(curl_);
}


void evb::bu::RUproxy::superFragmentCallback(toolbox::mem::Reference* bufRef)
{
  try
  {
    do
    {
      toolbox::mem::Reference* nextRef = bufRef->getNextReference();
      bufRef->setNextReference(0);

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

        const uint32_t nbSuperFragments = dataBlockMsg->nbSuperFragments;
        const uint32_t lastEventNumber = dataBlockMsg->evbIds[nbSuperFragments-1].eventNumber();
        if ( index.ruTid == evm_.tid )
        {
          if ( lastEventNumber > fragmentMonitoring_.lastEventNumberFromEVM )
            fragmentMonitoring_.lastEventNumberFromEVM = lastEventNumber;
        }
        else
        {
          if ( lastEventNumber > fragmentMonitoring_.lastEventNumberFromRUs )
            fragmentMonitoring_.lastEventNumberFromRUs = lastEventNumber;
        }
        fragmentMonitoring_.payload += payload;
        fragmentMonitoring_.payloadPerRU[index.ruTid] += payload;
        ++fragmentMonitoring_.i2oCount;
        if ( dataBlockMsg->blockNb == dataBlockMsg->nbBlocks )
        {
          fragmentMonitoring_.logicalCount += nbSuperFragments;
          fragmentMonitoring_.logicalCountPerRU[index.ruTid] += nbSuperFragments;
        }
      }

      {
        boost::mutex::scoped_lock sl(dataBlockMapMutex_);

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

        const uint16_t builderId = resourceManager_->underConstruction(dataBlockMsg);

        if ( dataBlockPos->second->isComplete() )
        {
          eventBuilder_->addSuperFragment(builderId,dataBlockPos->second);
          dataBlockMap_.erase(dataBlockPos);
        }
      }

      bufRef = nextRef;

    } while ( bufRef );
  }
  catch(xcept::Exception& e)
  {
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::SuperFragment,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::SuperFragment,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


void evb::bu::RUproxy::startProcessing()
{
  resetMonitoringCounters();

  doProcessing_ = true;
  requestFragmentsWL_->submit(requestFragmentsAction_);
}


void evb::bu::RUproxy::drain()
{
  while ( requestFragmentsActive_ || !dataBlockMap_.empty() ) ::usleep(1000);
}


void evb::bu::RUproxy::stopProcessing()
{
  doProcessing_ = false;
  while ( requestFragmentsActive_ ) ::usleep(1000);

  {
    boost::mutex::scoped_lock sl(dataBlockMapMutex_);
    dataBlockMap_.clear();
  }
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
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'requestFragments'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::RUproxy::requestFragments(toolbox::task::WorkLoop*)
{
  ::usleep(1000);

  requestFragmentsActive_ = true;

  try
  {
    uint16_t buResourceId;
    uint16_t eventsToDiscard;
    const uint32_t msgSize = sizeof(msg::ReadoutMsg)+sizeof(I2O_TID);

    while ( resourceManager_->getResourceId(buResourceId,eventsToDiscard) )
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
      if ( buResourceId > 0 )
        readoutMsg->nbRequests = configuration_->eventsPerRequest;
      else
        readoutMsg->nbRequests = 0;
      readoutMsg->nbDiscards   = eventsToDiscard;
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
  catch(std::exception& e)
  {
    requestFragmentsActive_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    requestFragmentsActive_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  requestFragmentsActive_ = false;

  return doProcessing_;
}


void evb::bu::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
  requestCount_ = 0;
  fragmentCount_ = 0;
  fragmentCountPerRU_.clear();
  payloadPerRU_.clear();

  items.add("requestCount", &requestCount_);
  items.add("fragmentCount", &fragmentCount_);
  items.add("fragmentCountPerRU", &fragmentCountPerRU_);
  items.add("payloadPerRU", &payloadPerRU_);
}


void evb::bu::RUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    requestCount_ = requestMonitoring_.logicalCount;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    fragmentMonitoring_.incompleteSuperFragments = dataBlockMap_.size();
    fragmentCount_ = fragmentMonitoring_.logicalCount;

    fragmentCountPerRU_.clear();
    fragmentCountPerRU_.reserve(fragmentMonitoring_.logicalCountPerRU.size());
    for (CountsPerRU::const_iterator it = fragmentMonitoring_.logicalCountPerRU.begin(),
           itEnd = fragmentMonitoring_.logicalCountPerRU.end();
         it != itEnd; ++it)
    {
      fragmentCountPerRU_.push_back(it->second);
    }

    payloadPerRU_.clear();
    payloadPerRU_.reserve(fragmentMonitoring_.payloadPerRU.size());
    for (CountsPerRU::const_iterator it = fragmentMonitoring_.payloadPerRU.begin(),
           itEnd = fragmentMonitoring_.payloadPerRU.end();
         it != itEnd; ++it)
    {
      payloadPerRU_.push_back(it->second);
    }
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
    fragmentMonitoring_.incompleteSuperFragments = 0;
    fragmentMonitoring_.payload = 0;
    fragmentMonitoring_.logicalCount = 0;
    fragmentMonitoring_.i2oCount = 0;
    fragmentMonitoring_.logicalCountPerRU.clear();
    fragmentMonitoring_.payloadPerRU.clear();
  }
}


void evb::bu::RUproxy::configure()
{
  {
    boost::mutex::scoped_lock sl(dataBlockMapMutex_);
    dataBlockMap_.clear();
  }

  getApplicationDescriptors();

  resetMonitoringCounters();
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
                  "Failed to get I2O TID for this application", e);
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

  evmURL_ = evm_.descriptor->getContextDescriptor()->getURL()
    + "/" + evm_.descriptor->getURN();
}


uint32_t evb::bu::RUproxy::getTotalEventsInLumiSection(const uint32_t lumiSection)
{
  uint32_t eventCount = 0;
  std::ostringstream url;
  url << evmURL_ << "/eventCountForLumiSection?ls=" << lumiSection;
  curl_easy_setopt(curl_, CURLOPT_URL, url.str().c_str());
  const CURLcode result = curl_easy_perform(curl_);

  if (result == CURLE_OK)
  {
    try
    {
      eventCount = boost::lexical_cast<uint32_t>(curlBuffer_);
    }
    catch(boost::bad_lexical_cast& e)
    {
      std::ostringstream oss;
      oss << "Received bad response from EVM: " << curlBuffer_;
      XCEPT_RAISE(exception::DiskWriting,oss.str());
    }
  }
  else
  {
    std::ostringstream oss;
    oss << "Failed to get total event count for lumi section " << lumiSection;
    oss << " from EVM at " << url.str() <<": ";
    oss << curl_easy_strerror(result);
    XCEPT_RAISE(exception::DiskWriting,oss.str());
  }

  curlBuffer_.clear();

  return eventCount;
}


int evb::bu::RUproxy::curlWriter(char* data, size_t size, size_t nmemb, std::string* buffer)
{
  int result = 0;
  if (buffer) {
    buffer->append(data, size * nmemb);
    result = size * nmemb;
  }
  return result;
}


cgicc::div evb::bu::RUproxy::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("RUproxy"));

  {
    table table;
    table.set("title","Statistics of super fragments received from the EVM/RUs.");

    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);

    table.add(tr()
              .add(td("last event number from EVM"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.lastEventNumberFromEVM))));
    table.add(tr()
              .add(td("last event number from RUs"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.lastEventNumberFromRUs))));
    table.add(tr()
              .add(td("# incomplete super fragments"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.incompleteSuperFragments))));
    table.add(tr()
              .add(th("Event data").set("colspan","2")));
    table.add(tr()
              .add(td("payload (MB)"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.payload / 1000000))));
    table.add(tr()
              .add(td("logical count"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.logicalCount))));
    table.add(tr()
              .add(td("I2O count"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.i2oCount))));
    div.add(table);
  }
  {
    table table;
    table.set("title","Statistics of requests sent to the EVM.");

    boost::mutex::scoped_lock sl(requestMonitoringMutex_);

    table.add(tr()
              .add(th("Event requests").set("colspan","2")));
    table.add(tr()
              .add(td("payload (kB)"))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.payload / 1000))));
    table.add(tr()
              .add(td("logical count"))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.logicalCount))));
    table.add(tr()
              .add(td("I2O count"))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.i2oCount))));
    div.add(table);
  }

  div.add(getStatisticsPerRU());

  return div;
}


cgicc::table evb::bu::RUproxy::getStatisticsPerRU() const
{
  using namespace cgicc;

  boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);

  table table;
  table.set("title","Statistics of received super fragments and total payload per EVM/RU.");

  table.add(tr()
            .add(th("Statistics per RU").set("colspan","4")));
  table.add(tr()
            .add(td("Instance"))
            .add(td("TID"))
            .add(td("Fragments"))
            .add(td("Payload (MB)")));

  CountsPerRU::const_iterator it, itEnd;
  for (it=fragmentMonitoring_.logicalCountPerRU.begin(),
         itEnd = fragmentMonitoring_.logicalCountPerRU.end();
       it != itEnd; ++it)
  {
    try
    {
      xdaq::ApplicationDescriptor* ru = i2o::utils::getAddressMap()->getApplicationDescriptor(it->first);
      const std::string url = ru->getContextDescriptor()->getURL() + "/" + ru->getURN();

      const std::string label = (it->first == evm_.tid) ? "EVM" :
        "RU "+boost::lexical_cast<std::string>(ru->getInstance());

      table.add(tr()
                .add(td()
                     .add(a(label).set("href",url).set("target","_blank")))
                .add(td(boost::lexical_cast<std::string>(it->first)))
                .add(td(boost::lexical_cast<std::string>(it->second)))
                .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.payloadPerRU.at(it->first) / 1000000))));
    }
    catch (xdaq::exception::ApplicationDescriptorNotFound& e)
    {
      const std::string label = (it->first == evm_.tid) ? "EVM" : "RU";

      table.add(tr()
                .add(td(label))
                .add(td(boost::lexical_cast<std::string>(it->first)))
                .add(td(boost::lexical_cast<std::string>(it->second)))
                .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.payloadPerRU.at(it->first) / 1000000))));
    }
  }
  return table;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
