#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/I2OMessages.h"
#include "xcept/tools.h"

#include <algorithm>
#include <math.h>
#include <string.h>


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
bu_(bu),
configuration_(bu->getConfiguration()),
throttleResources_(0),
freeResourceFIFO_("freeResourceFIFO"),
blockedResourceFIFO_("blockedResourceFIFO"),
lumiSectionAccountFIFO_("lumiSectionAccountFIFO"),
currentLumiSectionAccount_(new LumiSectionAccount(0))
{
  resetMonitoringCounters();
}


void evb::bu::ResourceManager::underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*& dataBlockMsg)
{
  boost::mutex::scoped_lock sl(allocatedResourcesMutex_);

  const AllocatedResources::iterator pos = allocatedResources_.find(dataBlockMsg->buResourceId);

  if ( pos == allocatedResources_.end() )
  {
    std::ostringstream oss;
    oss << "The buResourceId " << dataBlockMsg->buResourceId;
    oss << " received from RU tid " << ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;
    oss << " is not in the allocated resources" ;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( pos->second.empty() )
  {
    // first answer defines the EvBids handled by this resource
    for (uint32_t i=0; i < dataBlockMsg->nbSuperFragments; ++i)
    {
      pos->second.push_back(dataBlockMsg->evbIds[i]);
      incrementEventsInLumiSection(dataBlockMsg->evbIds[i].lumiSection());
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
      oss << " from RU tid " << ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;
      oss << " with an inconsistent number of super fragments: expected " << pos->second.size();
      oss << ", but got " << dataBlockMsg->nbSuperFragments;
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
  }
}


void evb::bu::ResourceManager::incrementEventsInLumiSection(const uint32_t lumiSection)
{
  if ( configuration_->dropEventData ) return;

  boost::mutex::scoped_lock sl(currentLumiSectionAccountMutex_);

  if ( lumiSection < currentLumiSectionAccount_->lumiSection )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << currentLumiSectionAccount_->lumiSection;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  if ( lumiSection > currentLumiSectionAccount_->lumiSection )
  {
    if ( currentLumiSectionAccount_->lumiSection > 0 )
      lumiSectionAccountFIFO_.enqWait(currentLumiSectionAccount_);

    // backfill any skipped lumi sections
    for (uint32_t ls = currentLumiSectionAccount_->lumiSection+1;
         ls < lumiSection; ++ls)
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
  if ( currentLumiSectionAccount_->lumiSection > 0 &&
    time(0) > (currentLumiSectionAccount_->startTime + lumiSectionTimeout_) )
  {
    lumiSectionAcount.swap(currentLumiSectionAccount_);
    currentLumiSectionAccount_.reset( new LumiSectionAccount(lumiSectionAcount->lumiSection + 1) );
    return true;
  }

  return false;
}


void evb::bu::ResourceManager::eventCompleted(const EventPtr& event)
{
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

      if ( throttleResources_ > 0 )
      {
        blockedResourceFIFO_.enqWait(event->buResourceId());
        --throttleResources_;
      }
      else
        freeResourceFIFO_.enqWait(event->buResourceId());
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

  if ( !gotResource && throttleResources_ < 0 && blockedResourceFIFO_.deq(buResourceId) )
  {
    gotResource = true;
    ++throttleResources_;
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
{}


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

  if ( currentLumiSectionAccount_->lumiSection > 0 )
  {
    lumiSectionAccountFIFO_.enqWait(currentLumiSectionAccount_);
    currentLumiSectionAccount_.reset( new LumiSectionAccount(0) );
  }
}


void evb::bu::ResourceManager::monitorDiskUsage(DiskUsagePtr& diskUsage)
{
  diskUsageMonitors_.push_back(diskUsage);
}


void evb::bu::ResourceManager::getDiskUsages()
{
  if ( diskUsageMonitors_.empty() ) return;

  float overThreshold = 0;

  for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
        it != itEnd; ++it)
  {
    (*it)->update();

    overThreshold = std::max(overThreshold,(*it)->overThreshold());
  }

  const int16_t resourceIdsToBlock = round(overThreshold * nbResources_) - blockedResourceFIFO_.elements();
  throttleResources_ += resourceIdsToBlock;
}


void evb::bu::ResourceManager::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEventsInBU_ = 0;
  nbEventsBuilt_ = 0;
  eventRate_ = 0;
  bandwidth_ = 0;
  eventSize_ = 0;
  eventSizeStdDev_ = 0;

  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEventsBuilt", &nbEventsBuilt_);
  items.add("eventRate", &eventRate_);
  items.add("bandwidth", &bandwidth_);
  items.add("eventSize", &eventSize_);
  items.add("eventSizeStdDev", &eventSizeStdDev_);
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
  getDiskUsages();

  boost::mutex::scoped_lock sl(eventMonitoringMutex_);

  nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
  nbEventsBuilt_ = eventMonitoring_.nbEventsBuilt;
  eventRate_ = eventMonitoring_.perf.logicalRate();
  bandwidth_ = eventMonitoring_.perf.bandwidth();
  eventSize_ = eventMonitoring_.perf.size();
  eventSizeStdDev_ = eventMonitoring_.perf.sizeStdDev();

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

  lumiSectionAccountFIFO_.resize(configuration_->lumiSectionFIFOCapacity);
  lumiSectionTimeout_ = configuration_->lumiSectionTimeout;

  nbResources_ = std::max(1U,
    configuration_->maxEvtsUnderConstruction.value_ /
    configuration_->eventsPerRequest.value_);
  freeResourceFIFO_.resize(nbResources_);
  blockedResourceFIFO_.resize(nbResources_);

  for (uint32_t buResourceId = 0; buResourceId < nbResources_; ++buResourceId)
  {
    assert( freeResourceFIFO_.enq(buResourceId) );
  }

  diskUsageMonitors_.clear();

  resetMonitoringCounters();
}


void evb::bu::ResourceManager::printHtml(xgi::Output *out) const
{
  const std::_Ios_Fmtflags originalFlags=out->flags();
  const int originalPrecision=out->precision();

  *out << "<div>"                                                 << std::endl;
  *out << "<p>ResourceManager</p>"                                << std::endl;
  *out << "<table>"                                               << std::endl;

  {
    boost::mutex::scoped_lock sl(eventMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events built</td>"                               << std::endl;
    *out << "<td>" << eventMonitoring_.nbEventsBuilt << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events in BU</td>"                               << std::endl;
    *out << "<td>" << eventMonitoring_.nbEventsInBU << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># outstanding requests</td>"                       << std::endl;
    *out << "<td>" << eventMonitoring_.outstandingRequests << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;

    out->setf(std::ios::fixed);
    out->precision(2);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << bandwidth_.value_ / 1e6 << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    out->precision(4);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << eventRate_.value_ << "</td>"                  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>event size (kB)</td>"                              << std::endl;
    *out << "<td>" << eventSize_.value_ / 1e3 <<
      " +/- " << eventSizeStdDev_.value_ / 1e3 << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  out->setf(std::ios::fixed);
  out->precision(1);

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Output disk usage</th>"              << std::endl;
  *out << "</tr>"                                                 << std::endl;
  for ( DiskUsageMonitors::const_iterator it = diskUsageMonitors_.begin(), itEnd = diskUsageMonitors_.end();
        it != itEnd; ++it)
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>" << (*it)->path() << "</td>"                      << std::endl;
    *out << "<td>" << (*it)->relDiskUsage()*100 << "% of "
      << (*it)->diskSizeGB() << " GB</td>"                          << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  out->flags(originalFlags);
  out->precision(originalPrecision);

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  freeResourceFIFO_.printHtml(out, bu_->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  blockedResourceFIFO_.printHtml(out, bu_->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  lumiSectionAccountFIFO_.printHtml(out, bu_->getURN());
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
