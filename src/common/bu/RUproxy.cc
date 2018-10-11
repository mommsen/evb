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

#include <limits>
#include <string.h>


evb::bu::RUproxy::RUproxy
(
  BU* bu,
  std::shared_ptr<EventBuilder> eventBuilder,
  std::shared_ptr<ResourceManager> resourceManager
) :
  bu_(bu),
  eventBuilder_(eventBuilder),
  resourceManager_(resourceManager),
  msgPool_(bu->getMsgPool()),
  configuration_(bu->getConfiguration()),
  doProcessing_(false),
  requestFragmentsActive_(false),
  tid_(0),
  roundTripTimeSampling_(0)
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
  if ( requestFragmentsWL_ && requestFragmentsWL_->isActive() )
    requestFragmentsWL_->cancel();
}


void evb::bu::RUproxy::superFragmentCallback(toolbox::mem::Reference* bufRef)
{
  toolbox::mem::Reference* nextRef = 0;

  try
  {
    do
    {
      nextRef = bufRef->getNextReference();
      bufRef->setNextReference(0);

      const I2O_MESSAGE_FRAME* stdMsg =
        (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
      const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg =
        (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
      const uint32_t payload = stdMsg->MessageSize << 2;

      Index index;
      index.ruTid = stdMsg->InitiatorAddress;
      index.buResourceId = dataBlockMsg->buResourceId;

      {
        boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);

        ++fragmentMonitoring_.perf.i2oCount;
        fragmentMonitoring_.perf.sumOfSizes += payload;
        fragmentMonitoring_.perf.sumOfSquares += payload*payload;

        CountsPerRU::iterator countsPerRuPos = fragmentMonitoring_.countsPerRU.lower_bound(index.ruTid);
        if ( countsPerRuPos == fragmentMonitoring_.countsPerRU.end() ||
             ( fragmentMonitoring_.countsPerRU.key_comp()(index.ruTid,countsPerRuPos->first)) )
        {
          countsPerRuPos = fragmentMonitoring_.countsPerRU.insert(countsPerRuPos, CountsPerRU::value_type(index.ruTid,StatsPerRU()));
        }
        countsPerRuPos->second.payload += payload;

        if ( dataBlockMsg->blockNb == 1 ) //only the first block contains the EvBid
        {
          const uint64_t now = getTimeStamp();
          const uint64_t deltaT = now>dataBlockMsg->timeStampNS ? now-dataBlockMsg->timeStampNS : 0;
          countsPerRuPos->second.roundTripTime = (roundTripTimeSampling_*deltaT) + (1-roundTripTimeSampling_)*countsPerRuPos->second.roundTripTime;

          const uint32_t nbSuperFragments = dataBlockMsg->nbSuperFragments;
          if ( nbSuperFragments > 1 )
          {
            const uint32_t lastEventNumber = dataBlockMsg->evbIds[nbSuperFragments-1].eventNumber();

            if ( index.ruTid == evm_.tid )
            {
              fragmentMonitoring_.lastEventNumberFromEVM = lastEventNumber;
            }
            else
            {
              fragmentMonitoring_.lastEventNumberFromRUs = lastEventNumber;
            }

            fragmentMonitoring_.perf.logicalCount += nbSuperFragments;
            countsPerRuPos->second.logicalCount += nbSuperFragments;
          }
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
            std::ostringstream msg;
            msg << "Received a first super-fragment block from RU tid " << index.ruTid;
            msg << " for BU resource id " << index.buResourceId;
            msg << " which is already block number " <<  dataBlockMsg->blockNb;
            msg << " of " << dataBlockMsg->nbBlocks;
            XCEPT_RAISE(exception::SuperFragment, msg.str());
          }

          FragmentChainPtr dataBlock( new FragmentChain(dataBlockMsg->nbBlocks) );
          dataBlockPos = dataBlockMap_.insert(dataBlockPos, DataBlockMap::value_type(index,dataBlock));
        }

        const uint16_t builderId = resourceManager_->underConstruction(dataBlockMsg);
        const bool superFragmentComplete = dataBlockPos->second->append(bufRef);
        bufRef = nextRef;

        if ( superFragmentComplete )
        {
          eventBuilder_->addSuperFragment(builderId,dataBlockPos->second);
          dataBlockMap_.erase(dataBlockPos);
        }
      }
    } while ( bufRef );
  }
  catch(xcept::Exception& e)
  {
    if ( bufRef ) bufRef->release();
    if ( nextRef ) nextRef->release();
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    if ( bufRef ) bufRef->release();
    if ( nextRef ) nextRef->release();
    XCEPT_DECLARE(exception::SuperFragment,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    if ( bufRef ) bufRef->release();
    if ( nextRef ) nextRef->release();
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
}


void evb::bu::RUproxy::startProcessingWorkLoop()
{
  try
  {
    requestFragmentsWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("requestFragments"), "waiting" );

    requestFragmentsAction_ =
      toolbox::task::bind(this, &evb::bu::RUproxy::requestFragments,
                          bu_->getIdentifier("ruProxyRequestFragments") );

    if ( ! requestFragmentsWL_->isActive() )
      requestFragmentsWL_->activate();
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'requestFragments'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::RUproxy::requestFragments(toolbox::task::WorkLoop*)
{
  if ( ! doProcessing_ ) return false;

  requestFragmentsActive_ = true;

  try
  {
    sendRequests();
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

  ::usleep(1000000/configuration_->maxRequestRate);

  return doProcessing_;
}


void evb::bu::RUproxy::sendRequests()
{
  ResourceManager::BUresources resources;
  resourceManager_->getAllAvailableResources(resources);
  const uint32_t nbRequests = resources.size();

  if ( nbRequests > 0 )
  {
    const uint32_t msgSize = sizeof(msg::ReadoutMsg) + nbRequests*sizeof(msg::EventRequest);
    uint32_t nbEventsRequested = 0;

    toolbox::mem::Reference* rqstBufRef = 0;
    do
    {
      try
      {
        rqstBufRef = toolbox::mem::getMemoryPoolFactory()->
          getFrame(msgPool_, msgSize);
        rqstBufRef->setDataSize(msgSize);
      }
      catch(toolbox::mem::exception::Exception)
      {
        rqstBufRef = 0;
        ::usleep(100);
      }
    } while ( !rqstBufRef );

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
    readoutMsg->nbRequests   = nbRequests;

    const uint64_t now = getTimeStamp();
    unsigned char* payload = (unsigned char*)&readoutMsg->requests[0];
    for ( ResourceManager::BUresources::const_iterator it = resources.begin(), itEnd = resources.end();
          it != itEnd; ++it )
    {
      msg::EventRequest* eventRequest = (msg::EventRequest*)payload;
      eventRequest->msgSize      = sizeof(msg::EventRequest);
      eventRequest->buTid        = tid_;
      eventRequest->priority     = it->priority;
      eventRequest->timeStampNS  = now;
      eventRequest->buResourceId = it->id;
      eventRequest->nbRequests   = it->id>0 ? configuration_->eventsPerRequest.value_ : 0;;
      eventRequest->nbDiscards   = it->eventsToDiscard;
      eventRequest->nbRUtids     = 0; // will be filled by EVM

      nbEventsRequested += eventRequest->nbRequests;
      payload += sizeof(msg::EventRequest);
    }

    // Send the request to the EVM
    uint32_t retries = 0;
    try
    {
      retries = bu_->postMessage(rqstBufRef,evm_.descriptor);
    }
    catch(exception::I2O& e)
    {
      {
        boost::mutex::scoped_lock sl(requestMonitoringMutex_);
        requestMonitoring_.perf.retryCount += retries;
      }

      std::ostringstream msg;
      msg << "Failed to send message to EVM TID ";
      msg << evm_.tid;
      XCEPT_RETHROW(exception::I2O, msg.str(), e);
    }

    {
      boost::mutex::scoped_lock sl(requestMonitoringMutex_);

      requestMonitoring_.perf.sumOfSizes += msgSize;
      requestMonitoring_.perf.sumOfSquares += msgSize*msgSize;
      requestMonitoring_.perf.logicalCount += nbEventsRequested;
      ++requestMonitoring_.perf.i2oCount;
      requestMonitoring_.perf.retryCount += retries;
    }
  }
}


uint64_t evb::bu::RUproxy::getTimeStamp() const
{
  if ( configuration_->roundTripTimeSamples == 0U )
    return 0;
  else
    return evb::getTimeStamp();
}


void evb::bu::RUproxy::appendMonitoringItems(InfoSpaceItems& items)
{
  requestRate_ = 0;
  requestRetryRate_ = 0;
  fragmentRate_ = 0;
  fragmentCountPerRU_.clear();
  payloadPerRU_.clear();
  slowestRUtid_ = 0;

  items.add("requestRate", &requestRate_);
  items.add("requestRetryRate", &requestRetryRate_);
  items.add("fragmentRate", &fragmentRate_);
  items.add("fragmentCountPerRU", &fragmentCountPerRU_);
  items.add("payloadPerRU", &payloadPerRU_);
  items.add("slowestRUtid", &slowestRUtid_);
}


void evb::bu::RUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);

    const double deltaT = requestMonitoring_.perf.deltaT();
    requestMonitoring_.throughput = requestMonitoring_.perf.throughput(deltaT);
    requestMonitoring_.requestRate = requestMonitoring_.perf.logicalRate(deltaT);
    requestMonitoring_.i2oRate = requestMonitoring_.perf.i2oRate(deltaT);
    requestMonitoring_.retryRate = requestMonitoring_.perf.retryRate(deltaT);
    requestMonitoring_.packingFactor = requestMonitoring_.perf.packingFactor();
    requestMonitoring_.perf.reset();
    requestRate_ = requestMonitoring_.i2oRate;
    requestRetryRate_ = requestMonitoring_.retryRate;
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);

    const double deltaT = fragmentMonitoring_.perf.deltaT();
    fragmentMonitoring_.incompleteSuperFragments = dataBlockMap_.size();
    fragmentMonitoring_.throughput = fragmentMonitoring_.perf.throughput(deltaT);
    fragmentMonitoring_.fragmentRate = fragmentMonitoring_.perf.logicalRate(deltaT);
    fragmentMonitoring_.i2oRate = fragmentMonitoring_.perf.i2oRate(deltaT);
    fragmentMonitoring_.packingFactor = fragmentMonitoring_.perf.packingFactor();
    fragmentRate_ = fragmentMonitoring_.i2oRate;

    fragmentCountPerRU_.clear();
    fragmentCountPerRU_.reserve(fragmentMonitoring_.countsPerRU.size());
    payloadPerRU_.clear();
    payloadPerRU_.reserve(fragmentMonitoring_.countsPerRU.size());
    slowestRUtid_ = 0;

    uint32_t minRoundTripTime = std::numeric_limits<uint32_t>::max();
    uint32_t maxRoundTripTime = 0;
    for (CountsPerRU::iterator it = fragmentMonitoring_.countsPerRU.begin(),
           itEnd = fragmentMonitoring_.countsPerRU.end();
         it != itEnd; ++it)
    {
      if ( minRoundTripTime > it->second.roundTripTime )
      {
        minRoundTripTime = it->second.roundTripTime;
      }
      if ( maxRoundTripTime < it->second.roundTripTime )
      {
        maxRoundTripTime = it->second.roundTripTime;
        slowestRUtid_ = it->first;
      }
    }

    for (CountsPerRU::iterator it = fragmentMonitoring_.countsPerRU.begin(),
           itEnd = fragmentMonitoring_.countsPerRU.end();
         it != itEnd; ++it)
    {
      fragmentCountPerRU_.push_back(it->second.logicalCount);
      payloadPerRU_.push_back(it->second.payload);
      it->second.deltaTns = it->second.roundTripTime - minRoundTripTime;
    }
    fragmentMonitoring_.perf.reset();
  }
}


void evb::bu::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    requestMonitoring_.requestRate = 0;
    requestMonitoring_.requestRetryRate = 0;
    requestMonitoring_.perf.reset();
  }
  {
    boost::mutex::scoped_lock sl(fragmentMonitoringMutex_);
    fragmentMonitoring_.lastEventNumberFromEVM = 0;
    fragmentMonitoring_.lastEventNumberFromRUs = 0;
    fragmentMonitoring_.incompleteSuperFragments = 0;
    fragmentMonitoring_.fragmentRate = 0;
    fragmentMonitoring_.perf.reset();
    fragmentMonitoring_.countsPerRU.clear();
  }
}


void evb::bu::RUproxy::configure()
{
  {
    boost::mutex::scoped_lock sl(dataBlockMapMutex_);
    dataBlockMap_.clear();
  }

  roundTripTimeSampling_ = configuration_->roundTripTimeSamples>0U ? 1./configuration_->roundTripTimeSamples : 0;

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

    std::set<const xdaq::ApplicationDescriptor*> evmDescriptors;

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
      std::ostringstream msg;
      msg << "Failed to get application descriptor of EVM";
      msg << configuration_->evmInstance.toString();
      XCEPT_RETHROW(exception::Configuration, msg.str(), e);
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
  std::ostringstream url;
  url << evmURL_ << "/eventCountForLumiSection?ls=" << lumiSection;
  return getValueFromEVM(url.str());
}


uint32_t evb::bu::RUproxy::getLatestLumiSection()
{
  return getValueFromEVM(evmURL_+"/getLatestLumiSection");
}


uint32_t evb::bu::RUproxy::getValueFromEVM(const std::string& url)
{
  boost::mutex::scoped_lock sl(curlMutex_);

  uint32_t value = 0;
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
  const CURLcode result = curl_easy_perform(curl_);

  if (result == CURLE_OK)
  {
    try
    {
      value = boost::lexical_cast<uint32_t>(curlBuffer_);
    }
    catch(boost::bad_lexical_cast& e)
    {
      std::ostringstream msg;
      msg << "Received bad response from EVM: " << curlBuffer_;
      XCEPT_RAISE(exception::DiskWriting,msg.str());
    }
  }
  else
  {
    std::ostringstream msg;
    msg << "Failed to get value from EVM at " << url <<": ";
    msg << curl_easy_strerror(result);
    XCEPT_RAISE(exception::DiskWriting,msg.str());
  }

  curlBuffer_.clear();

  return value;
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
              .add(td("throughput (MB/s)"))
              .add(td(doubleToString(fragmentMonitoring_.throughput / 1e6,2))));
    table.add(tr()
              .add(td("fragment rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.fragmentRate))));
    table.add(tr()
              .add(td("I2O rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(fragmentMonitoring_.i2oRate))));
    table.add(tr()
              .add(td("Fragments/I2O"))
              .add(td(doubleToString(fragmentMonitoring_.packingFactor,1))));
    div.add(table);
  }
  {
    table table;
    table.set("title","Statistics of requests sent to the EVM.");

    boost::mutex::scoped_lock sl(requestMonitoringMutex_);

    table.add(tr()
              .add(th("Event requests").set("colspan","2")));
    table.add(tr()
              .add(td("throughput (kB/s)"))
              .add(td(doubleToString(requestMonitoring_.throughput / 1e3,2))));
    table.add(tr()
              .add(td("request rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.requestRate))));
    table.add(tr()
              .add(td("I2O rate (Hz)"))
              .add(td(boost::lexical_cast<std::string>(requestMonitoring_.i2oRate))));
    table.add(tr()
              .add(td("I2O retry rate (Hz)"))
              .add(td(doubleToString(requestMonitoring_.retryRate,2))));
    table.add(tr()
              .add(td("Events requested/I2O"))
              .add(td(doubleToString(requestMonitoring_.packingFactor,1))));
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
            .add(th("Statistics per RU").set("colspan","5")));
  table.add(tr()
            .add(td("Instance"))
            .add(td("TID"))
            .add(td("Fragments"))
            .add(td("Payload (MB)"))
            .add(td("&Delta;T (ns)")));

  CountsPerRU::const_iterator it, itEnd;
  for (it=fragmentMonitoring_.countsPerRU.begin(),
         itEnd = fragmentMonitoring_.countsPerRU.end();
       it != itEnd; ++it)
  {
    try
    {
      const xdaq::ApplicationDescriptor* ru = i2o::utils::getAddressMap()->getApplicationDescriptor(it->first);
      const std::string url = ru->getContextDescriptor()->getURL() + "/" + ru->getURN();

      const std::string label = (it->first == evm_.tid) ? "EVM" :
        "RU "+boost::lexical_cast<std::string>(ru->getInstance());

      table.add(tr()
                .add(td()
                     .add(a(label).set("href",url).set("target","_blank")))
                .add(td(boost::lexical_cast<std::string>(it->first)))
                .add(td(boost::lexical_cast<std::string>(it->second.logicalCount)))
                .add(td(boost::lexical_cast<std::string>(it->second.payload / 1000000)))
                .add(td(boost::lexical_cast<std::string>(it->second.deltaTns))));
    }
    catch (xdaq::exception::ApplicationDescriptorNotFound& e)
    {
      const std::string label = (it->first == evm_.tid) ? "EVM" : "RU";

      table.add(tr()
                .add(td(label))
                .add(td(boost::lexical_cast<std::string>(it->first)))
                .add(td(boost::lexical_cast<std::string>(it->second.logicalCount)))
                .add(td(boost::lexical_cast<std::string>(it->second.payload / 1000000)))
                .add(td(boost::lexical_cast<std::string>(it->second.deltaTns))));
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
