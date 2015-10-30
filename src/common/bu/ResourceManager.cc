#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/I2OMessages.h"
#include "xcept/tools.h"

#include <algorithm>
#include <fstream>
#include <math.h>
#include <string.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
  bu_(bu),
  configuration_(bu->getConfiguration()),
  resourceFIFO_(bu,"resourceFIFO"),
  runNumber_(0),
  eventsToDiscard_(0),
  nbResources_(1),
  blockedResources_(1),
  currentPriority_(0),
  builderId_(0),
  fusHLT_(0),
  fusCloud_(0),
  fusQuarantined_(0),
  fusStale_(0),
  initiallyQueuedLS_(0),
  queuedLS_(0),
  queuedLSonFUs_(-1),
  oldestIncompleteLumiSection_(0),
  doProcessing_(false)
{
  startResourceMonitorWorkLoop();
  resetMonitoringCounters();
  eventMonitoring_.outstandingRequests = 0;
}


evb::bu::ResourceManager::~ResourceManager()
{
  if ( resourceMonitorWL_ && resourceMonitorWL_->isActive() )
    resourceMonitorWL_->cancel();
}


void evb::bu::ResourceManager::startResourceMonitorWorkLoop()
{
  try
  {
    resourceMonitorWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("resourceMonitor"), "waiting" );

    if ( ! resourceMonitorWL_->isActive() )
    {
      resourceMonitorWL_->activate();

      toolbox::task::ActionSignature* resourceMonitorAction =
        toolbox::task::bind(this, &evb::bu::ResourceManager::resourceMonitor,
                            bu_->getIdentifier("resourceMonitor") );

      resourceMonitorWL_->submit(resourceMonitorAction);
    }
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'resourceMonitor'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


uint16_t evb::bu::ResourceManager::underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*& dataBlockMsg)
{
  const BuilderResources::iterator pos = builderResources_.find(dataBlockMsg->buResourceId);
  const I2O_TID ruTid = ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;

  if ( pos == builderResources_.end() )
  {
    std::ostringstream msg;
    msg << "The buResourceId " << dataBlockMsg->buResourceId;
    msg << " received from RU tid " << ruTid;
    msg << " is not in the builder resources" ;
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  if ( dataBlockMsg->blockNb == 1 ) //only the first block contains the EvBid
  {
    if ( pos->second.evbIdList.empty() )
    {
      // first answer defines the EvBids handled by this resource
      for (uint32_t i=0; i < dataBlockMsg->nbSuperFragments; ++i)
      {
        const EvBid& evbId = dataBlockMsg->evbIds[i];
        pos->second.evbIdList.push_back(evbId);
        incrementEventsInLumiSection(evbId.lumiSection());
      }
      boost::mutex::scoped_lock sl(eventMonitoringMutex_);
      eventMonitoring_.nbEventsInBU += dataBlockMsg->nbSuperFragments;
      --eventMonitoring_.outstandingRequests;
    }
    else
    {
      // check consistency
      if ( pos->second.evbIdList.size() != dataBlockMsg->nbSuperFragments )
      {
        std::ostringstream msg;
        msg << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
        msg << " from RU tid " << ruTid;
        msg << " with an inconsistent number of super fragments: expected " << pos->second.evbIdList.size();
        msg << ", but got " << dataBlockMsg->nbSuperFragments;
        XCEPT_RAISE(exception::SuperFragment, msg.str());
      }
      uint32_t index = 0;
      for (EvBidList::const_iterator it = pos->second.evbIdList.begin(), itEnd = pos->second.evbIdList.end();
           it != itEnd; ++it)
      {
        if ( dataBlockMsg->evbIds[index] != *it )
        {
          std::ostringstream msg;
          msg << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
          msg << " from RU tid " << ruTid;
          msg << " with an inconsistent EvBid for super fragment " << index;
          msg << ": expected " << *it;
          msg << ", but got " << dataBlockMsg->evbIds[index];
          XCEPT_RAISE(exception::SuperFragment, msg.str());
        }
        if ( dataBlockMsg->evbIds[index].lumiSection() != it->lumiSection() )
        {
          std::ostringstream msg;
          msg << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
          msg << " from RU tid " << ruTid;
          msg << " with an inconsistent lumi section for super fragment " << index;
          msg << ": expected " << it->lumiSection();
          msg << ", but got " << dataBlockMsg->evbIds[index].lumiSection();
          XCEPT_RAISE(exception::SuperFragment, msg.str());
        }
        ++index;
      }
    }
  }

  return pos->second.builderId;
}


uint32_t evb::bu::ResourceManager::getOldestIncompleteLumiSection() const
{
  boost::mutex::scoped_lock sl(lsLatencyMutex_);
  return oldestIncompleteLumiSection_;
}


void evb::bu::ResourceManager::incrementEventsInLumiSection(const uint32_t lumiSection)
{
  if ( lumiSection == 0 ) return;

  boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

  LumiSectionAccounts::const_iterator pos = lumiSectionAccounts_.find(lumiSection);

  if ( pos == lumiSectionAccounts_.end() )
  {
    // backfill any skipped lumi sections
    const uint32_t newestLumiSection = lumiSectionAccounts_.empty() ? 0 : lumiSectionAccounts_.rbegin()->first;
    for ( uint32_t ls = newestLumiSection+1; ls <= lumiSection; ++ls )
    {
      LumiSectionAccountPtr lumiAccount( new LumiSectionAccount(ls) );
      pos = lumiSectionAccounts_.insert(LumiSectionAccounts::value_type(ls,lumiAccount)).first;
    }
  }

  if ( pos == lumiSectionAccounts_.end() )
  {
    std::ostringstream msg;
    msg << "Received an event from an earlier lumi section " << lumiSection;
    msg << " that has already been closed.";
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  ++(pos->second->nbEvents);
  ++(pos->second->nbIncompleteEvents);
}


void evb::bu::ResourceManager::eventCompletedForLumiSection(const uint32_t lumiSection)
{
  if ( lumiSection == 0 ) return;

  boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

  LumiSectionAccounts::iterator pos = lumiSectionAccounts_.find(lumiSection);

  if ( pos == lumiSectionAccounts_.end() )
  {
    std::ostringstream msg;
    msg << "Completed an event from an unknown lumi section " << lumiSection;
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  --(pos->second->nbIncompleteEvents);
}


bool evb::bu::ResourceManager::getNextLumiSectionAccount
(
  LumiSectionAccountPtr& lumiSectionAccount,
  const bool completeLumiSectionsOnly
)
{
  boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

  const LumiSectionAccounts::iterator oldestLumiSection = lumiSectionAccounts_.begin();
  bool foundCompleteLS = false;

  if ( oldestLumiSection == lumiSectionAccounts_.end() ) return false;

  if ( !completeLumiSectionsOnly )
  {
    lumiSectionAccount = oldestLumiSection->second;
    lumiSectionAccounts_.erase(oldestLumiSection);
    return true;
  }

  if ( blockedResources_ == nbResources_ )
  {
    // do not expire any lumi section if all resources are blocked
    time_t now = time(0);
    for ( LumiSectionAccounts::const_iterator it = lumiSectionAccounts_.begin(), itEnd = lumiSectionAccounts_.end();
          it != itEnd; ++it)
    {
      it->second->startTime = now;
    }

    return false;
  }

  if ( oldestLumiSection->second->nbIncompleteEvents == 0 )
  {
    if ( lumiSectionAccounts_.size() > 1 // there are newer lumi sections
         || !doProcessing_ )
    {
      lumiSectionAccount = oldestLumiSection->second;
      lumiSectionAccounts_.erase(oldestLumiSection);
      foundCompleteLS = true;
    }
    else // check if the current lumi section timed out
    {
      uint32_t lumiSection = oldestLumiSection->second->lumiSection;
      const uint32_t lumiDuration = time(0) - oldestLumiSection->second->startTime;
      if ( lumiSection > 0 && lumiSectionTimeout_ > 0 && lumiDuration > lumiSectionTimeout_ )
      {
        std::ostringstream msg;
        msg.setf(std::ios::fixed);
        msg.precision(1);
        msg << "Lumi section " << lumiSection << " timed out after " << lumiDuration << "s";
        LOG4CPLUS_INFO(bu_->getApplicationLogger(), msg.str());
        lumiSectionAccount = oldestLumiSection->second;
        lumiSectionAccounts_.erase(oldestLumiSection);
        foundCompleteLS = true;

        // Insert the next lumi section
        ++lumiSection;
        LumiSectionAccountPtr lumiAccount( new LumiSectionAccount(lumiSection) );
        lumiSectionAccounts_.insert(LumiSectionAccounts::value_type(lumiSection,lumiAccount));
      }
    }
  }

  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);
    oldestIncompleteLumiSection_ = lumiSectionAccounts_.begin()->first;
  }
  return foundCompleteLS;
}


void evb::bu::ResourceManager::eventCompleted(const EventPtr& event)
{
  eventCompletedForLumiSection(event->getEventInfo()->lumiSection());

  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  const uint32_t eventSize = event->getEventInfo()->eventSize();
  eventMonitoring_.perf.sumOfSizes += eventSize;
  eventMonitoring_.perf.sumOfSquares += eventSize*eventSize;
  ++eventMonitoring_.perf.logicalCount;
  ++eventMonitoring_.nbEventsBuilt;
}


void evb::bu::ResourceManager::discardEvent(const EventPtr& event)
{
  const BuilderResources::iterator pos = builderResources_.find(event->buResourceId());

  if ( pos == builderResources_.end() )
  {
    std::ostringstream msg;
    msg << "The buResourceId " << event->buResourceId();
    msg << " is not in the builder resources" ;
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  pos->second.evbIdList.remove(event->getEvBid());

  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    --eventMonitoring_.nbEventsInBU;
    ++eventsToDiscard_;
  }

  if ( pos->second.evbIdList.empty() )
  {
    pos->second.builderId = -1;
    boost::mutex::scoped_lock sl(resourceFIFOmutex_);
    resourceFIFO_.enqWait(pos);
  }
}


bool evb::bu::ResourceManager::getResourceId(uint16_t& buResourceId, uint16_t& priority, uint16_t& eventsToDiscard)
{
  BuilderResources::iterator pos;
  if ( resourceFIFO_.deq(pos) )
  {
    if ( pos->second.blocked )
    {
      ::usleep(configuration_->sleepTimeBlocked*1000);

      boost::mutex::scoped_lock sl(resourceFIFOmutex_);
      resourceFIFO_.enqWait(pos);
    }
    else
    {
      boost::mutex::scoped_lock sl(eventMonitoringMutex_);

      pos->second.builderId = (++builderId_) % configuration_->numberOfBuilders;
      buResourceId = pos->first;
      priority = currentPriority_;
      eventsToDiscard = eventsToDiscard_;
      eventsToDiscard_ = 0;
      ++eventMonitoring_.outstandingRequests;
      return true;
    }
  }

  if ( blockedResources_ == nbResources_ && eventsToDiscard_ > 0 )
  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);

    buResourceId = 0;
    priority = 0;
    eventsToDiscard = eventsToDiscard_;
    eventsToDiscard_ = 0;
    return true;
  }

  return false;
}


void evb::bu::ResourceManager::startProcessing()
{
  resetMonitoringCounters();
  doProcessing_ = true;
  runNumber_ = bu_->getStateMachine()->getRunNumber();
}


void evb::bu::ResourceManager::drain()
{
  doProcessing_ = false;
}


void evb::bu::ResourceManager::stopProcessing()
{
  doProcessing_ = false;
}


float evb::bu::ResourceManager::getAvailableResources()
{
  boost::mutex::scoped_lock sl(resourceSummaryMutex_);

  if ( resourceSummary_.empty() ) return nbResources_;

  const std::time_t lastWriteTime = boost::filesystem::last_write_time(resourceSummary_);
  if ( configuration_->staleResourceTime > 0U &&
       (std::time(0) - lastWriteTime) > configuration_->staleResourceTime )
  {
    std::ostringstream msg;
    msg << resourceSummary_ << " has not been updated in the last ";
    msg << configuration_->staleResourceTime << "s";
    handleResourceSummaryFailure(msg.str());
    return 0;
  }

  uint32_t lsLatency = 0;
  float resourcesFromFUs =  0;
  try
  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(resourceSummary_.string(), pt);
    fusHLT_ = pt.get<int>("active_resources");
    fusCloud_ = pt.get<int>("cloud");
    fusQuarantined_ = pt.get<int>("quarantined");
    fusStale_ = pt.get<int>("stale_resources");
    resourcesFromFUs = (fusHLT_ == 0) ? 0 :
      std::max(1.0, fusHLT_ * configuration_->resourcesPerCore);

    queuedLSonFUs_ = pt.get<int>("activeRunNumQueuedLS");
    const uint32_t activeFURun = pt.get<int>("activeFURun");
    const uint32_t activeRunCMSSWMaxLS = std::max(0,pt.get<int>("activeRunCMSSWMaxLS"));
    if ( activeFURun == runNumber_ && activeRunCMSSWMaxLS > 0 )
    {
      if ( oldestIncompleteLumiSection_ >= activeRunCMSSWMaxLS )
        queuedLS_ = oldestIncompleteLumiSection_ - activeRunCMSSWMaxLS;
      if ( initiallyQueuedLS_ == 0 )
        initiallyQueuedLS_ = queuedLS_;
      else if ( queuedLS_ > initiallyQueuedLS_ )
        lsLatency = queuedLS_ - initiallyQueuedLS_;
      else
        lsLatency = 0;
    }
  }
  catch(boost::property_tree::ptree_error& e)
  {
    std::ostringstream msg;
    msg << "Failed to parse " << resourceSummary_.string() << ": ";
    msg << e.what();
    handleResourceSummaryFailure(msg.str());
    return 0;
  }
  resourceSummaryFailureAlreadyNotified_ = false;

  if ( lsLatency > configuration_->lumiSectionLatencyHigh.value_ )
  {
    if ( ! resourceLimitiationAlreadyNotified_ )
    {
      std::ostringstream msg;
      msg << "There are " << lsLatency << " LS queued for the FUs which is more than the allowed latency of "
        << configuration_->lumiSectionLatencyHigh.value_ << " LS";
      LOG4CPLUS_WARN(bu_->getApplicationLogger(), msg.str());
      resourceLimitiationAlreadyNotified_ = true;
    }
    return 0;
  }
  if ( queuedLSonFUs_ > static_cast<int32_t>(configuration_->maxFuLumiSectionLatency.value_) )
  {
    if ( ! resourceLimitiationAlreadyNotified_ )
    {
      std::ostringstream msg;
      msg << "There are " << queuedLSonFUs_ << " LS queued on FUs which is more than the allowed latency of "
        << configuration_->maxFuLumiSectionLatency.value_ << " LS";
      LOG4CPLUS_WARN(bu_->getApplicationLogger(), msg.str());
      resourceLimitiationAlreadyNotified_ = true;
    }
    return 0;
  }

  if ( lsLatency > configuration_->lumiSectionLatencyLow.value_ )
  {
    if ( ! resourceLimitiationAlreadyNotified_ )
    {
      std::ostringstream msg;
      msg << "Throttling requests as there are " << lsLatency << " LS queued for FUs which is above the low water mark of "
        << configuration_->lumiSectionLatencyLow.value_ << " LS";
      LOG4CPLUS_WARN(bu_->getApplicationLogger(), msg.str());
      resourceLimitiationAlreadyNotified_ = true;
    }

    const float throttle = 1 - ( static_cast<float>(lsLatency - configuration_->lumiSectionLatencyLow) /
                                 (configuration_->lumiSectionLatencyHigh - configuration_->lumiSectionLatencyLow) );
    return std::min(resourcesFromFUs,static_cast<float>(nbResources_)) * throttle;
  }

  if ( resourcesFromFUs >= static_cast<float>(nbResources_) )
  {
    resourceLimitiationAlreadyNotified_ = false;
    return nbResources_;
  }
  else
  {
    return resourcesFromFUs;
  }
}


void evb::bu::ResourceManager::handleResourceSummaryFailure(const std::string& msg)
{
  if ( resourceSummaryFailureAlreadyNotified_ ) return;

  LOG4CPLUS_ERROR(bu_->getApplicationLogger(),msg);

  XCEPT_DECLARE(exception::DiskWriting,sentinelError,msg);
  bu_->notifyQualified("error",sentinelError);

  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);

    fusHLT_ = 0;
    fusCloud_ = 0;
    fusStale_ = 0;
    fusQuarantined_ = 0;
    initiallyQueuedLS_ = 0;
    queuedLS_ = 0;
    queuedLSonFUs_ = -1;
    resourceSummaryFailureAlreadyNotified_ = true;
  }
}


void evb::bu::ResourceManager::updateDiskUsages()
{
  boost::mutex::scoped_lock sl(diskUsageMonitorsMutex_);

  for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
        it != itEnd; ++it)
  {
    (*it)->update();

    const boost::filesystem::path path = (*it)->path();
    boost::filesystem::path::const_iterator pathIter = path.begin();
    while ( pathIter != path.end() )
    {
      if ( boost::algorithm::icontains(pathIter->string(),"ramdisk") )
      {
        ramDiskSizeInGB_ = (*it)->diskSizeGB();
        ramDiskUsed_ = (*it)->relDiskUsage();
        break;
      }
      ++pathIter;
    }
  }
}


float evb::bu::ResourceManager::getOverThreshold()
{
  boost::mutex::scoped_lock sl(diskUsageMonitorsMutex_);

  if ( diskUsageMonitors_.empty() ) return 0;

  float overThreshold = 0;

  for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
        it != itEnd; ++it)
  {
    overThreshold = std::max(overThreshold,(*it)->overThreshold());
  }

  return overThreshold;
}


bool evb::bu::ResourceManager::resourceMonitor(toolbox::task::WorkLoop*)
{
  std::string msg = "Failed to update available resources";

  try
  {
    const float availableResources = getAvailableResources();

    if ( doProcessing_ )
    {
      updateResources(availableResources);
      changeStatesBasedOnResources();

      if ( configuration_->usePriorities )
      {
        boost::mutex::scoped_lock sl(eventMonitoringMutex_);
        currentPriority_ = getPriority();
      }
    }
  }
  catch(xcept::Exception &e)
  {
    XCEPT_DECLARE_NESTED(exception::FFF, sentinelException, msg, e);
    bu_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::FFF,
                  sentinelException, msg+": "+e.what());
    bu_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::FFF,
                  sentinelException, msg+": "+"unkown exception");
    bu_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  ::sleep(1);

  return true;
}


void evb::bu::ResourceManager::updateResources(const float availableResources)
{
  uint32_t resourcesToBlock;
  const float overThreshold = getOverThreshold();

  if ( overThreshold >= 1 )
  {
    resourcesToBlock = nbResources_;
  }
  else
  {
    const uint32_t usableResources = round( (1-overThreshold) * availableResources );
    resourcesToBlock = nbResources_ < usableResources ? 0 : nbResources_ - usableResources;
  }

  {
    boost::mutex::scoped_lock sl(builderResourcesMutex_);

    BuilderResources::reverse_iterator rit = builderResources_.rbegin();
    const BuilderResources::reverse_iterator ritEnd = builderResources_.rend();
    while ( blockedResources_ < resourcesToBlock && rit != ritEnd )
    {
      if ( !rit->second.blocked )
      {
        rit->second.blocked = true;
        ++blockedResources_;
      }
      ++rit;
    }

    BuilderResources::iterator it = builderResources_.begin();
    const BuilderResources::iterator itEnd = builderResources_.end();
    while ( blockedResources_ > resourcesToBlock && it != itEnd )
    {
      if ( it->second.blocked )
      {
        it->second.blocked = false;
        --blockedResources_;
      }
      ++it;
    }
  }
}


uint16_t evb::bu::ResourceManager::getPriority()
{
  boost::mutex::scoped_lock sl(diskUsageMonitorsMutex_);

  const float weight = (1-pow(1-ramDiskUsed_,2)) * evb::LOWEST_PRIORITY;
  if ( blockedResources_ > 0 )
    return round(weight);
  else
    return floor(weight);
}


void evb::bu::ResourceManager::changeStatesBasedOnResources()
{
  bool allFUsQuarantined;
  bool allFUsStale;
  bool fusInCloud;
  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);
    allFUsQuarantined = (fusHLT_ == 0U) && (fusQuarantined_ > 0U);
    allFUsStale = (fusHLT_ == 0U) && (fusStale_ > 0U);
    fusInCloud = (fusCloud_ > 0U) && (fusStale_ == 0U);
  }

  uint32_t outstandingRequests;
  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    outstandingRequests = std::max(0,eventMonitoring_.outstandingRequests);
  }

  if ( allFUsQuarantined )
  {
    XCEPT_DECLARE(exception::FFF, e,
                  "All FU cores in the appliance are quarantined, i.e. HLT is failing on all of them");
    bu_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  else if ( allFUsStale )
  {
    XCEPT_DECLARE(exception::FFF, e,
                  "All FUs in the appliance are reporting a stale file handle");
    bu_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  else if ( blockedResources_ == 0U || outstandingRequests > configuration_->numberOfBuilders )
  {
    bu_->getStateMachine()->processFSMEvent( Release() );
  }
  else if ( blockedResources_ == nbResources_ )
  {
    if ( fusInCloud )
      bu_->getStateMachine()->processFSMEvent( Clouded() );
    else
      bu_->getStateMachine()->processFSMEvent( Block() );
  }
  else if ( blockedResources_ > 0U && outstandingRequests == 0 )
  {
    if ( fusInCloud )
      bu_->getStateMachine()->processFSMEvent( Misted() );
    else
      bu_->getStateMachine()->processFSMEvent( Throttle() );
  }
}


void evb::bu::ResourceManager::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEventsInBU_ = 0;
  nbEventsBuilt_ = 0;
  eventRate_ = 0;
  bandwidth_ = 0;
  eventSize_ = 0;
  eventSizeStdDev_ = 0;
  outstandingRequests_ = 0;
  nbTotalResources_ = 1;
  nbBlockedResources_ = 1;
  priority_ = 0;
  fuSlotsHLT_ = 0;
  fuSlotsCloud_ = 0;
  fuSlotsQuarantined_ = 0;
  fuSlotsStale_ = 0;
  queuedLumiSections_ = 0;
  queuedLumiSectionsOnFUs_ = -1;
  ramDiskSizeInGB_ = 0;
  ramDiskUsed_ = 0;

  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEventsBuilt", &nbEventsBuilt_);
  items.add("eventRate", &eventRate_);
  items.add("bandwidth", &bandwidth_);
  items.add("eventSize", &eventSize_);
  items.add("eventSizeStdDev", &eventSizeStdDev_);
  items.add("outstandingRequests", &outstandingRequests_);
  items.add("nbTotalResources", &nbTotalResources_);
  items.add("nbBlockedResources", &nbBlockedResources_);
  items.add("priority", &priority_);
  items.add("fuSlotsHLT", &fuSlotsHLT_);
  items.add("fuSlotsCloud", &fuSlotsCloud_);
  items.add("fuSlotsQuarantined", &fuSlotsQuarantined_);
  items.add("fuSlotsStale", &fuSlotsStale_);
  items.add("queuedLumiSections", &queuedLumiSections_);
  items.add("queuedLumiSectionsOnFUs", &queuedLumiSectionsOnFUs_);
  items.add("ramDiskSizeInGB", &ramDiskSizeInGB_);
  items.add("ramDiskUsed", &ramDiskUsed_);
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
  updateDiskUsages();

  nbTotalResources_ = nbResources_;
  nbBlockedResources_ = blockedResources_;

  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);

    fuSlotsHLT_ = fusHLT_;
    fuSlotsCloud_ = fusCloud_;
    fuSlotsQuarantined_ = fusQuarantined_;
    fuSlotsStale_ = fusStale_;
    queuedLumiSections_ = queuedLS_;
    queuedLumiSectionsOnFUs_ = queuedLSonFUs_;
  }
  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);

    const double deltaT = eventMonitoring_.perf.deltaT();
    nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
    nbEventsBuilt_ = eventMonitoring_.nbEventsBuilt;
    eventRate_ = eventMonitoring_.perf.logicalRate(deltaT);
    bandwidth_ = eventMonitoring_.perf.bandwidth(deltaT);
    eventSize_ = eventMonitoring_.perf.size();
    eventSizeStdDev_ = eventMonitoring_.perf.sizeStdDev();
    outstandingRequests_ = std::max(0,eventMonitoring_.outstandingRequests);
    priority_ = currentPriority_;

    eventMonitoring_.perf.reset();
  }
}


void evb::bu::ResourceManager::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(lsLatencyMutex_);

    oldestIncompleteLumiSection_ = 0;
    queuedLS_ = 0;
    initiallyQueuedLS_ = 0;
  }
  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);

    eventMonitoring_.nbEventsInBU = 0;
    eventMonitoring_.nbEventsBuilt = 0;
    eventMonitoring_.perf.reset();
  }
}


uint32_t evb::bu::ResourceManager::getEventSize() const
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  return eventMonitoring_.perf.size();
}


uint32_t evb::bu::ResourceManager::getEventRate() const
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  const double deltaT = eventMonitoring_.perf.deltaT();
  return eventMonitoring_.perf.logicalRate(deltaT);
}


uint32_t evb::bu::ResourceManager::getBandwidth() const
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  const double deltaT = eventMonitoring_.perf.deltaT();
  return eventMonitoring_.perf.bandwidth(deltaT);
}


uint32_t evb::bu::ResourceManager::getNbEventsInBU() const
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  return eventMonitoring_.nbEventsInBU;
}


uint64_t evb::bu::ResourceManager::getNbEventsBuilt() const
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  return eventMonitoring_.nbEventsBuilt;
}


void evb::bu::ResourceManager::configure()
{
  resourceFIFO_.clear();
  lumiSectionAccounts_.clear();

  eventsToDiscard_ = 0;
  eventMonitoring_.outstandingRequests = 0;

  lumiSectionTimeout_ = configuration_->lumiSectionTimeout;

  nbResources_ = std::max(1U,
                          configuration_->maxEvtsUnderConstruction.value_ /
                          configuration_->eventsPerRequest.value_);
  resourceFIFO_.resize(nbResources_);

  configureResources();
  configureResourceSummary();
  configureDiskUsageMonitors();
}


void evb::bu::ResourceManager::configureResources()
{
  boost::mutex::scoped_lock sl(builderResourcesMutex_);
  if (configuration_->dropEventData)
    blockedResources_ = 0;
  else
    blockedResources_ = nbResources_;

  builderResources_.clear();
  for (uint32_t resourceId = 1; resourceId <= nbResources_; ++resourceId)
  {
    ResourceInfo resourceInfo;
    resourceInfo.builderId = -1;
    resourceInfo.blocked = !configuration_->dropEventData;
    std::pair<BuilderResources::iterator,bool> result =
      builderResources_.insert(BuilderResources::value_type(resourceId,resourceInfo));
    if ( ! result.second )
    {
      std::ostringstream msg;
      msg << "Failed to insert resource " << resourceId << " into builder resource map";
      XCEPT_RAISE(exception::DiskWriting, msg.str());
    }
    assert( resourceFIFO_.enq(result.first) );
  }
}


void evb::bu::ResourceManager::configureResourceSummary()
{
  boost::mutex::scoped_lock sl(resourceSummaryMutex_);

  resourceSummary_.clear();
  resourceSummaryFailureAlreadyNotified_ = false;
  resourceLimitiationAlreadyNotified_ = false;

  if ( configuration_->dropEventData ) return;

  if ( !configuration_->ignoreResourceSummary && !configuration_->deleteRawDataFiles )
  {
    resourceSummary_ = boost::filesystem::path(configuration_->rawDataDir.value_) / configuration_->resourceSummaryFileName.value_;
    if ( !boost::filesystem::exists(resourceSummary_) )
    {
      std::ostringstream msg;
      msg << "Resource summary file " << resourceSummary_ << " does not exist";
      resourceSummary_.clear();
      XCEPT_RAISE(exception::DiskWriting, msg.str());
    }
  }
}


void evb::bu::ResourceManager::configureDiskUsageMonitors()
{
  boost::mutex::scoped_lock sl(diskUsageMonitorsMutex_);

  diskUsageMonitors_.clear();

  if ( configuration_->dropEventData ) return;

  DiskUsagePtr rawDiskUsage(
    new DiskUsage(configuration_->rawDataDir.value_,configuration_->rawDataLowWaterMark,configuration_->rawDataHighWaterMark,configuration_->deleteRawDataFiles)
  );
  diskUsageMonitors_.push_back(rawDiskUsage);

  if ( configuration_->metaDataDir != configuration_->rawDataDir )
  {
    DiskUsagePtr metaDiskUsage(
      new DiskUsage(configuration_->metaDataDir.value_,configuration_->metaDataLowWaterMark,configuration_->metaDataHighWaterMark,false)
    );
    diskUsageMonitors_.push_back(metaDiskUsage);
  }
}


cgicc::div evb::bu::ResourceManager::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("ResourceManager"));

  {
    table table;
    table.set("title","Statistics of built events. If the number of outstanding requests is larger than 0, the BU waits for data.");

    boost::mutex::scoped_lock sl(eventMonitoringMutex_);

    table.add(tr()
              .add(td("# events built"))
              .add(td(boost::lexical_cast<std::string>(eventMonitoring_.nbEventsBuilt))));
    table.add(tr()
              .add(td("# events in BU"))
              .add(td(boost::lexical_cast<std::string>(eventMonitoring_.nbEventsInBU))));
    table.add(tr()
              .add(td("# outstanding requests"))
              .add(td(boost::lexical_cast<std::string>(eventMonitoring_.outstandingRequests))));
    table.add(tr()
              .add(td("priority of requests"))
              .add(td(boost::lexical_cast<std::string>(currentPriority_))));
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(2);
      str << bandwidth_.value_ / 1e6;
      table.add(tr()
                .add(td("throughput (MB/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::scientific);
      str.precision(4);
      str << eventRate_.value_;
      table.add(tr()
                .add(td("rate (events/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << eventSize_.value_ / 1e3 << " +/- " << eventSizeStdDev_.value_ / 1e3;
      table.add(tr()
                .add(td("event size (kB)"))
                .add(td(str.str())));
    }

    div.add(table);
  }

  {
    table table;
    table.set("title","If no FU slots are available or the output disk is full, no events are requested unless 'dropEventData' is set to true in the configuration.");

    {
      boost::mutex::scoped_lock sl(lsLatencyMutex_);

      table.add(tr()
                .add(td("# FU slots available"))
                .add(td(boost::lexical_cast<std::string>(fusHLT_))));
      table.add(tr()
                .add(td("# FU slots used for cloud"))
                .add(td(boost::lexical_cast<std::string>(fusCloud_))));
      table.add(tr()
                .add(td("# FU slots quarantined"))
                .add(td(boost::lexical_cast<std::string>(fusQuarantined_))));
      table.add(tr()
                .add(td("# stale FU slots"))
                .add(td(boost::lexical_cast<std::string>(fusStale_))));
      table.add(tr()
                .add(td("# queued lumi sections for FUs"))
                .add(td(boost::lexical_cast<std::string>(queuedLS_))));
      table.add(tr()
                .add(td("# queued lumi sections on FUs"))
                .add(td(boost::lexical_cast<std::string>(queuedLSonFUs_))));
      table.add(tr()
                .add(td("# blocked resources"))
                .add(td(boost::lexical_cast<std::string>(blockedResources_)+"/"+boost::lexical_cast<std::string>(nbResources_))));
    }
    {
      boost::mutex::scoped_lock sl(diskUsageMonitorsMutex_);

      if ( ! diskUsageMonitors_.empty() )
      {
        table.add(tr()
                  .add(th("Output disk usage").set("colspan","2")));

        for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
              it != itEnd; ++it)
        {
          std::ostringstream str;
          str.setf(std::ios::fixed);
          str.precision(1);
          str << (*it)->relDiskUsage()*100 << "% of " << (*it)->diskSizeGB() << " GB";
          table.add(tr()
                    .add(td((*it)->path().string()))
                    .add(td(str.str())));
        }
      }
    }

    div.add(table);
  }

  {
    cgicc::div resources;
    resources.set("title","A resource is used to request a number of events ('eventsPerRequest'). Resources are blocked if not enough FU cores are available or if the output disk becomes full. The number of resources per FU slot is governed by the configuration parameter 'resourcesPerCore'.");

    resources.add(resourceFIFO_.getHtmlSnipped());

    div.add(resources);
  }

  {
    boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

    if ( ! lumiSectionAccounts_.empty() )
    {
      cgicc::table table;
      table.set("title","List of lumi sections for which events are currently being built.");

      table.add(tr()
                .add(th("Active lumi sections").set("colspan","4")));
      table.add(tr()
                .add(td("LS"))
                .add(td("#events"))
                .add(td("#incomplete"))
                .add(td("age (s)")));

      time_t now = time(0);
      for ( LumiSectionAccounts::const_iterator it = lumiSectionAccounts_.begin(), itEnd = lumiSectionAccounts_.end();
            it != itEnd; ++it)
      {
        std::ostringstream str;
        str.setf(std::ios::fixed);
        str.precision(1);
        str << now - it->second->startTime;

        table.add(tr()
                  .add(td(boost::lexical_cast<std::string>(it->first)))
                  .add(td(boost::lexical_cast<std::string>(it->second->nbEvents)))
                  .add(td(boost::lexical_cast<std::string>(it->second->nbIncompleteEvents)))
                  .add(td(str.str())));
      }

      div.add(table);
    }
  }

  div.add(br());
  div.add(a("display resource table")
          .set("href","/"+bu_->getURN().toString()+"/resourceTable").set("target","_blank"));

  return div;
}


cgicc::div evb::bu::ResourceManager::getHtmlSnippedForResourceTable() const
{
  using namespace cgicc;

  table table;
  table.set("class","xdaq-table-vertical");

  tr row;
  row.add(th("id")).add(th("builder"));
  for (uint32_t request = 0; request < configuration_->eventsPerRequest; ++request)
    row.add(th(boost::lexical_cast<std::string>(request)));
  table.add(row);

  const std::string colspan = boost::lexical_cast<std::string>(configuration_->eventsPerRequest+1);

  {
    boost::mutex::scoped_lock sl(builderResourcesMutex_);

    for (BuilderResources::const_iterator it = builderResources_.begin(), itEnd = builderResources_.end();
          it != itEnd; ++it)
    {
      tr row;
      row.add(td((boost::lexical_cast<std::string>(it->first))));

      if ( it->second.blocked )
      {
        row.add(td("BLOCKED").set("colspan",colspan));
      }
      else if ( it->second.builderId == -1 )
      {
        row.add(td("free").set("colspan",colspan));
      }
      else if ( it->second.evbIdList.empty() )
      {
        row.add(td("outstanding").set("colspan",colspan));
      }
      else
      {
        row.add(td(boost::lexical_cast<std::string>(it->second.builderId)));

        uint32_t colCount = 0;
        for (EvBidList::const_iterator evbIt = it->second.evbIdList.begin(), evbItEnd = it->second.evbIdList.end();
             evbIt != evbItEnd; ++evbIt)
        {
          std::ostringstream evbid;
          evbid << *evbIt;
          row.add(td(evbid.str()));
          ++colCount;
        }
        for ( uint32_t i = colCount; i < configuration_->eventsPerRequest; ++i)
        {
          row.add(td(" "));
        }
      }

      table.add(row);
    }
  }

  cgicc::div div;
  div.add(table);
  return div;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
