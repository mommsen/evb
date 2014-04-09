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

#include <boost/lexical_cast.hpp>


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
bu_(bu),
configuration_(bu->getConfiguration()),
nbResources_(1),
resourcesToBlock_(1),
freeResourceFIFO_(bu,"freeResourceFIFO"),
blockedResourceFIFO_(bu,"blockedResourceFIFO"),
lumiSectionAccountFIFO_(bu,"lumiSectionAccountFIFO")
{
  resetMonitoringCounters();
}


void evb::bu::ResourceManager::underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*& dataBlockMsg)
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

  if ( pos->second.empty() )
  {
    // first answer defines the EvBids handled by this resource
    for (uint32_t i=0; i < dataBlockMsg->nbSuperFragments; ++i)
    {
      pos->second.push_back(dataBlockMsg->evbIds[i]);
    }
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    eventMonitoring_.nbEventsInBU += dataBlockMsg->nbSuperFragments;
    --eventMonitoring_.outstandingRequests;
  }
  else
  {
    // check consistency
    if ( pos->second.size() != dataBlockMsg->nbSuperFragments )
    {
      std::ostringstream oss;
      oss << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
      oss << " from RU tid " << ruTid;
      oss << " with an inconsistent number of super fragments: expected " << pos->second.size();
      oss << ", but got " << dataBlockMsg->nbSuperFragments;
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
    uint32_t index = 0;
    for (EvBidList::const_iterator it = pos->second.begin(), itEnd = pos->second.end();
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
}


void evb::bu::ResourceManager::incrementEventsInLumiSection(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);

  if ( ! currentLumiSectionAccount_.get() ) return;

  const uint32_t currentLumiSection = currentLumiSectionAccount_->lumiSection;
  if ( lumiSection < currentLumiSection )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << currentLumiSection;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( lumiSection > currentLumiSection )
  {
    lumiSectionAccountFIFO_.enqWait(currentLumiSectionAccount_);

    // backfill any skipped lumi sections
    for (uint32_t ls = currentLumiSection+1; ls < lumiSection; ++ls)
    {
      lumiSectionAccountFIFO_.enqWait(LumiSectionAccountPtr( new LumiSectionAccount(ls) ));
    }

    currentLumiSectionAccount_.reset( new LumiSectionAccount(lumiSection) );
  }

  ++(currentLumiSectionAccount_->nbEvents);
}


bool evb::bu::ResourceManager::getNextLumiSectionAccount(LumiSectionAccountPtr& lumiSectionAcount)
{
  if ( lumiSectionAccountFIFO_.deq(lumiSectionAcount) ) return true;

  boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);
  if ( currentLumiSectionAccount_.get() )
  {
    if ( blockedResourceFIFO_.full() )
    {
      //delay the expiry of current lumi section if we are not requesting any events
      currentLumiSectionAccount_->startTime = time(0);
    }
    else
    {
      const uint32_t currentLumiSection = currentLumiSectionAccount_->lumiSection;
      const uint32_t lumiDuration = time(0) - currentLumiSectionAccount_->startTime;
      if ( currentLumiSection > 0 && lumiDuration > lumiSectionTimeout_ )
      {
        std::ostringstream msg;
        msg.setf(std::ios::fixed);
        msg.precision(1);
        msg << "Lumi section " <<  currentLumiSection << " timed out after " << lumiDuration << "s";
        LOG4CPLUS_INFO(bu_->getApplicationLogger(), msg.str());
        lumiSectionAcount.swap(currentLumiSectionAccount_);
        currentLumiSectionAccount_.reset( new LumiSectionAccount(currentLumiSection+1) );
        return true;
      }
    }
  }
  return false;
}


uint32_t evb::bu::ResourceManager::getCurrentLumiSection()
{
  boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);
  if ( currentLumiSectionAccount_.get() )
    return currentLumiSectionAccount_->lumiSection;
  else
    return 0;
}


void evb::bu::ResourceManager::eventCompleted(const EventPtr& event)
{
  incrementEventsInLumiSection(event->lumiSection());

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

    pos->second.remove(event->getEvBid());
    if ( pos->second.empty() )
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


bool evb::bu::ResourceManager::getResourceId(uint32_t& buResourceId)
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

      if ( ! allocatedResources_.insert(AllocatedResources::value_type(buResourceId,EvBidList())).second )
      {
        std::ostringstream oss;
        oss << "The buResourceId " << buResourceId;
        oss << " is already in the allocated resources, while it was also found in the free resources";
        XCEPT_RAISE(exception::EventOrder, oss.str());
      }
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
  if ( ! configuration_->dropEventData )
  {
    boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);
    currentLumiSectionAccount_.reset( new LumiSectionAccount(1) );
  }
}


void evb::bu::ResourceManager::drain()
{
  enqCurrentLumiSectionAccount();
  while ( ! lumiSectionAccountFIFO_.empty() ) { ::usleep(1000); }
}


void evb::bu::ResourceManager::stopProcessing()
{
  enqCurrentLumiSectionAccount();
}


void evb::bu::ResourceManager::enqCurrentLumiSectionAccount()
{
  boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);

  if ( currentLumiSectionAccount_.get() )
  {
    lumiSectionAccountFIFO_.enqWait(currentLumiSectionAccount_);
    currentLumiSectionAccount_.reset();
  }
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
  fuCoresAvailable_ = 0;

  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEventsBuilt", &nbEventsBuilt_);
  items.add("eventRate", &eventRate_);
  items.add("bandwidth", &bandwidth_);
  items.add("eventSize", &eventSize_);
  items.add("eventSizeStdDev", &eventSizeStdDev_);
  items.add("fuCoresAvailable", &fuCoresAvailable_);
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
  updateResources();

  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
  nbEventsBuilt_ = eventMonitoring_.nbEventsBuilt;
  eventRate_ = eventMonitoring_.perf.logicalRate();
  bandwidth_ = eventMonitoring_.perf.bandwidth();
  const uint32_t eventSize = eventMonitoring_.perf.size();
  if ( eventSize > 0 )
  {
    eventSize_ = eventSize;
    eventSizeStdDev_ = eventMonitoring_.perf.sizeStdDev();
  }

  eventMonitoring_.perf.reset();
}


void evb::bu::ResourceManager::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  eventMonitoring_.nbEventsInBU = 0;
  eventMonitoring_.nbEventsBuilt = 0;
  eventMonitoring_.outstandingRequests = 0;
  eventMonitoring_.perf.reset();
}


void evb::bu::ResourceManager::configure()
{
  freeResourceFIFO_.clear();
  blockedResourceFIFO_.clear();
  allocatedResources_.clear();
  lumiSectionAccountFIFO_.clear();
  diskUsageMonitors_.clear();
  resourceDirectory_.clear();

  lumiSectionAccountFIFO_.resize(configuration_->lumiSectionFIFOCapacity);
  lumiSectionTimeout_ = configuration_->lumiSectionTimeout;

  nbResources_ = std::max(1U,
    configuration_->maxEvtsUnderConstruction.value_ /
    configuration_->eventsPerRequest.value_);
  freeResourceFIFO_.resize(nbResources_);
  blockedResourceFIFO_.resize(nbResources_);

  if ( configuration_->dropEventData )
  {
    resourcesToBlock_ = 0;
    for (uint32_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId)
      assert( freeResourceFIFO_.enq(buResourceId) );
  }
  else
  {
    resourcesToBlock_ = nbResources_;
    for (uint32_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId)
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

  table table;

  {
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
  }
  table.add(tr()
    .add(td("# FU cores available"))
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

  table.add(tr()
    .add(td().set("colspan","2")
      .add(freeResourceFIFO_.getHtmlSnipped())));

  table.add(tr()
    .add(td().set("colspan","2")
      .add(blockedResourceFIFO_.getHtmlSnipped())));

  table.add(tr()
    .add(td().set("colspan","2")
      .add(lumiSectionAccountFIFO_.getHtmlSnipped())));

  cgicc::div div;
  div.add(p("ResourceManager"));
  div.add(table);
  return div;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
