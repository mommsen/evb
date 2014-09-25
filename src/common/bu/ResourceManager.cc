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


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
  bu_(bu),
  configuration_(bu->getConfiguration()),
  nbResources_(1),
  resourcesToBlock_(1),
  builderId_(0),
  freeResourceFIFO_(bu,"freeResourceFIFO"),
  blockedResourceFIFO_(bu,"blockedResourceFIFO"),
  draining_(false)
{
  resetMonitoringCounters();
}


uint16_t evb::bu::ResourceManager::underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*& dataBlockMsg)
{
  boost::mutex::scoped_lock sl(allocatedResourcesMutex_);

  const AllocatedResources::iterator pos = allocatedResources_.find(dataBlockMsg->buResourceId);
  const I2O_TID ruTid = ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;

  if ( pos == allocatedResources_.end() )
  {
    std::ostringstream oss;
    oss << "The buResourceId " << dataBlockMsg->buResourceId;
    oss << " received from RU tid " << ruTid;
    oss << " is not in the allocated resources" ;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( pos->second.evbIdList.empty() )
  {
    // first answer defines the EvBids handled by this resource
    pos->second.builderId = (++builderId_) % configuration_->numberOfBuilders;
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
      std::ostringstream oss;
      oss << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
      oss << " from RU tid " << ruTid;
      oss << " with an inconsistent number of super fragments: expected " << pos->second.evbIdList.size();
      oss << ", but got " << dataBlockMsg->nbSuperFragments;
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
    uint32_t index = 0;
    for (EvBidList::const_iterator it = pos->second.evbIdList.begin(), itEnd = pos->second.evbIdList.end();
         it != itEnd; ++it)
    {
      if ( dataBlockMsg->evbIds[index] != *it )
      {
        std::ostringstream oss;
        oss << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
        oss << " from RU tid " << ruTid;
        oss << " with an inconsistent EvBid for super fragment " << index;
        oss << ": expected " << *it;
        oss << ", but got " << dataBlockMsg->evbIds[index];
        XCEPT_RAISE(exception::SuperFragment, oss.str());
      }
      if ( dataBlockMsg->evbIds[index].lumiSection() != it->lumiSection() )
      {
        std::ostringstream oss;
        oss << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
        oss << " from RU tid " << ruTid;
        oss << " with an inconsistent lumi section for super fragment " << index;
        oss << ": expected " << it->lumiSection();
        oss << ", but got " << dataBlockMsg->evbIds[index].lumiSection();
        XCEPT_RAISE(exception::SuperFragment, oss.str());
      }
      ++index;
    }
  }
  return pos->second.builderId;
}


uint32_t evb::bu::ResourceManager::getOldestIncompleteLumiSection() const
{
  return oldestIncompleteLumiSection_;
}


void evb::bu::ResourceManager::incrementEventsInLumiSection(const uint32_t lumiSection)
{
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
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " that has already been closed.";
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  ++(pos->second->nbEvents);
  ++(pos->second->incompleteEvents);
}


void evb::bu::ResourceManager::eventCompletedForLumiSection(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

  LumiSectionAccounts::iterator pos = lumiSectionAccounts_.find(lumiSection);

  if ( pos == lumiSectionAccounts_.end() )
  {
    std::ostringstream oss;
    oss << "Completed an event from an unknown lumi section " << lumiSection;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  --(pos->second->incompleteEvents);
}


bool evb::bu::ResourceManager::getNextLumiSectionAccount(LumiSectionAccountPtr& lumiSectionAccount)
{
  boost::mutex::scoped_lock sl(lumiSectionAccountsMutex_);

  if ( lumiSectionAccounts_.empty() ) return false;

  if ( blockedResourceFIFO_.full() )
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

  const LumiSectionAccounts::iterator oldestLumiSection = lumiSectionAccounts_.begin();
  bool foundCompleteLS = false;

  if ( oldestLumiSection->second->incompleteEvents == 0 )
  {
    if ( lumiSectionAccounts_.size() > 1 // there are newer lumi sections
         || draining_ )
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

  oldestIncompleteLumiSection_ = lumiSectionAccounts_.begin()->first;

  return foundCompleteLS;
}


void evb::bu::ResourceManager::eventCompleted(const EventPtr& event)
{
  eventCompletedForLumiSection(event->lumiSection());

  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  const uint32_t eventSize = event->eventSize();
  eventMonitoring_.perf.sumOfSizes += eventSize;
  eventMonitoring_.perf.sumOfSquares += eventSize*eventSize;
  ++eventMonitoring_.perf.logicalCount;
  ++eventMonitoring_.nbEventsBuilt;
}


void evb::bu::ResourceManager::discardEvent(const EventPtr& event)
{
  {
    boost::mutex::scoped_lock sl(allocatedResourcesMutex_);

    const AllocatedResources::iterator pos = allocatedResources_.find(event->buResourceId());

    if ( pos == allocatedResources_.end() )
    {
      std::ostringstream oss;
      oss << "The buResourceId " << event->buResourceId();
      oss << " is not in the allocated resources" ;
      XCEPT_RAISE(exception::EventOrder, oss.str());
    }

    pos->second.evbIdList.remove(event->getEvBid());
    ++eventsToDiscard_;

    if ( pos->second.evbIdList.empty() )
    {
      allocatedResources_.erase(pos);

      if ( blockedResourceFIFO_.elements() < resourcesToBlock_ )
        blockedResourceFIFO_.enqWait( event->buResourceId() );
      else
        freeResourceFIFO_.enqWait( event->buResourceId() );
    }
  }

  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    --eventMonitoring_.nbEventsInBU;
  }
}


bool evb::bu::ResourceManager::getResourceId(uint16_t& buResourceId, uint16_t& eventsToDiscard)
{
  bool gotResource = freeResourceFIFO_.deq(buResourceId);

  if ( !gotResource && blockedResourceFIFO_.elements() > resourcesToBlock_ && blockedResourceFIFO_.deq(buResourceId) )
  {
    gotResource = true;
  }

  if ( gotResource )
  {
    {
      boost::mutex::scoped_lock sl(allocatedResourcesMutex_);

      if ( ! allocatedResources_.insert(AllocatedResources::value_type(buResourceId,ResourceInfo())).second )
      {
        std::ostringstream oss;
        oss << "The buResourceId " << buResourceId;
        oss << " is already in the allocated resources, while it was also found in the free resources";
        XCEPT_RAISE(exception::EventOrder, oss.str());
      }

      eventsToDiscard = eventsToDiscard_;
      eventsToDiscard_ = 0;
    }

    {
      boost::mutex::scoped_lock sl(eventMonitoringMutex_);
      ++eventMonitoring_.outstandingRequests;
    }

    return true;
  }

  return false;
}


void evb::bu::ResourceManager::startProcessing()
{
  oldestIncompleteLumiSection_ = 0;
  draining_ = false;
  resetMonitoringCounters();
}


void evb::bu::ResourceManager::drain()
{
  draining_ = true;
}


void evb::bu::ResourceManager::stopProcessing()
{
  draining_ = true;
}


float evb::bu::ResourceManager::getAvailableResources()
{
  if ( resourceDirectory_.empty() ) return nbResources_;

  uint32_t coreCount = 0;
  boost::filesystem::directory_iterator dirIter(resourceDirectory_);

  while ( dirIter != boost::filesystem::directory_iterator() )
  {
    const std::time_t lastWriteTime =
      boost::filesystem::last_write_time(*dirIter);
    if ( (std::time(0) - lastWriteTime) < configuration_->staleResourceTime )
    {
      std::ifstream boxFile( dirIter->path().string().c_str() );
      while (boxFile)
      {
        std::string line;
        std::getline(boxFile,line);
        std::size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
          const std::string key = line.substr(0,pos);
          if ( key == "idles" || key == "used" )
          {
            try
            {
              coreCount += boost::lexical_cast<uint32_t>(line.substr(pos+1));
            }
            catch(boost::bad_lexical_cast& e) {}
          }
        }
      }
    }
    ++dirIter;
  }

  fuCoresAvailable_ = coreCount;
  const float resourcesFromCores = coreCount * configuration_->resourcesPerCore;

  return ( resourcesFromCores > nbResources_ ? nbResources_ : resourcesFromCores );
}


void evb::bu::ResourceManager::updateResources()
{
  if ( diskUsageMonitors_.empty() ) return;

  float overThreshold = 0;

  for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
        it != itEnd; ++it)
  {
    (*it)->update();

    const boost::filesystem::path path = (*it)->path();
    boost::filesystem::path::const_iterator pathIter = path.begin();
    while ( pathIter != path.end() )
    {
      if ( boost::algorithm::icontains(*pathIter,"ramdisk") )
      {
        ramDiskSizeInGB_ = (*it)->diskSizeGB();
        ramDiskUsed_ = (*it)->relDiskUsage();
        break;
      }
      ++pathIter;
    }

    overThreshold = std::max(overThreshold,(*it)->overThreshold());
  }

  if ( overThreshold >= 1 )
  {
    resourcesToBlock_ = nbResources_;
  }
  else
  {
    const uint32_t usableResources = round( (1-overThreshold) * getAvailableResources() );
    resourcesToBlock_ = nbResources_ < usableResources ? 0 : nbResources_ - usableResources;
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
  nbTotalResources_ = 0;
  nbBlockedResources_ = 0;
  fuCoresAvailable_ = 0;
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
  items.add("fuCoresAvailable", &fuCoresAvailable_);
  items.add("ramDiskSizeInGB", &ramDiskSizeInGB_);
  items.add("ramDiskUsed", &ramDiskUsed_);
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
  updateResources();
  nbTotalResources_ = nbResources_;
  nbBlockedResources_ = blockedResourceFIFO_.elements();

  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);

    nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
    nbEventsBuilt_ = eventMonitoring_.nbEventsBuilt;
    eventRate_ = eventMonitoring_.perf.logicalRate();
    bandwidth_ = eventMonitoring_.perf.bandwidth();
    outstandingRequests_ = std::max(0,eventMonitoring_.outstandingRequests);
    eventSize_ = eventMonitoring_.perf.size();
    eventSizeStdDev_ = eventMonitoring_.perf.sizeStdDev();

    eventMonitoring_.perf.reset();
  }
}


void evb::bu::ResourceManager::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  eventMonitoring_.nbEventsInBU = 0;
  eventMonitoring_.nbEventsBuilt = 0;
  eventMonitoring_.perf.reset();
}


void evb::bu::ResourceManager::configure()
{
  freeResourceFIFO_.clear();
  blockedResourceFIFO_.clear();
  allocatedResources_.clear();
  lumiSectionAccounts_.clear();
  diskUsageMonitors_.clear();
  resourceDirectory_.clear();

  eventsToDiscard_ = 0;
  eventMonitoring_.outstandingRequests = 0;

  lumiSectionTimeout_ = configuration_->lumiSectionTimeout;

  nbResources_ = std::max(1U,
                          configuration_->maxEvtsUnderConstruction.value_ /
                          configuration_->eventsPerRequest.value_);
  freeResourceFIFO_.resize(nbResources_);
  blockedResourceFIFO_.resize(nbResources_);

  if ( configuration_->dropEventData )
  {
    resourcesToBlock_ = 0;
    for (uint16_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId)
      assert( freeResourceFIFO_.enq(buResourceId) );
  }
  else
  {
    resourcesToBlock_ = nbResources_;
    for (uint16_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId)
      assert( blockedResourceFIFO_.enq(buResourceId) );

    configureDiskUsageMonitors();
  }

  resetMonitoringCounters();
}


void evb::bu::ResourceManager::configureDiskUsageMonitors()
{
  resourceDirectory_ = boost::filesystem::path(configuration_->rawDataDir.value_) / "appliance" / "boxes";
  if ( ! boost::filesystem::is_directory(resourceDirectory_) )
    resourceDirectory_.clear();

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

    table.add(tr()
              .add(td("# FU slots available"))
              .add(td(boost::lexical_cast<std::string>(fuCoresAvailable_.value_))));

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

    div.add(table);
  }

  {
    cgicc::div resources;
    resources.set("title","A resource is used to request a number of events ('eventsPerRequest'). Resources are blocked if not enough FU cores are available or if the output disk becomes full. The number of resources per FU slot is governed by the configuration parameter 'resourcesPerCore'.");

    resources.add(freeResourceFIFO_.getHtmlSnipped());
    resources.add(blockedResourceFIFO_.getHtmlSnipped());

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
                  .add(td(boost::lexical_cast<std::string>(it->second->incompleteEvents)))
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
    boost::mutex::scoped_lock sl(allocatedResourcesMutex_);

    for ( uint16_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId )
    {
      tr row;
      row.add(td((boost::lexical_cast<std::string>(buResourceId))));

      const AllocatedResources::const_iterator pos = allocatedResources_.find(buResourceId);

      if ( pos != allocatedResources_.end() )
      {
        if ( pos->second.evbIdList.empty() )
        {
           row.add(td("outstanding").set("colspan",colspan));
        }
        else
        {
          row.add(td(boost::lexical_cast<std::string>(pos->second.builderId)));

          uint32_t colCount = 0;
          for ( EvBidList::const_iterator it = pos->second.evbIdList.begin(), itEnd = pos->second.evbIdList.end();
                it != itEnd; ++it)
          {
            std::ostringstream evbid;
            evbid << *it;
            row.add(td(evbid.str()));
            ++colCount;
          }
          for ( uint32_t i = colCount; i < configuration_->eventsPerRequest; ++i)
          {
            row.add(td(" "));
          }
        }
      }
      else
      {
        row.add(td("unused").set("colspan",colspan));
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
