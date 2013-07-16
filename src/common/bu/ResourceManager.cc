#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "xcept/tools.h"

#include <algorithm>
#include <string.h>


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
bu_(bu),
boost_(false),
throttle_(false),
freeResourceFIFO_("freeResourceFIFO"),
blockedResourceFIFO_("blockedResourceFIFO")
{
  resetMonitoringCounters();
}


void evb::bu::ResourceManager::underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg)
{
  boost::mutex::scoped_lock sl(allocatedResourcesMutex_);
  
  const AllocatedResources::iterator pos = allocatedResources_.find(dataBlockMsg->buResourceId);

  if ( pos == allocatedResources_.end() )
  {
    std::stringstream oss;
    
    oss << "The buResourceId " << dataBlockMsg->buResourceId;
    oss << " received from RU tid " << ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;
    oss << " is not in the allocated resources." ;
    
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }
  
  if ( pos->second.empty() )
  {
    // first answer defines the EvBids handled by this resource
    for (uint32_t i=0; i < dataBlockMsg->nbSuperFragments; ++i)
    {
      pos->second.push_back(dataBlockMsg->evbIds[i]);
    }
    eventMonitoring_.nbEventsInBU += dataBlockMsg->nbSuperFragments;
  }
  else
  {
    // check consistency
    if ( pos->second.size() != dataBlockMsg->nbSuperFragments )
    {
      std::stringstream oss;
    
      oss << "Received an I2O_DATA_BLOCK_MESSAGE_FRAME for buResourceId " << dataBlockMsg->buResourceId;
      oss << " from RU tid " << ((I2O_MESSAGE_FRAME*)dataBlockMsg)->InitiatorAddress;
      oss << " with an inconsistent number of super fragments: expected " << pos->second.size();
      oss << ", but got " << dataBlockMsg->nbSuperFragments;
      
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
  }
}


void evb::bu::ResourceManager::eventCompleted(const EventPtr event)
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  const uint32_t eventSize = event->eventSize();
  eventMonitoring_.perf.sumOfSizes += eventSize;
  eventMonitoring_.perf.sumOfSquares += eventSize*eventSize;
  ++eventMonitoring_.perf.logicalCount;
  ++eventMonitoring_.nbEventsBuilt;
}


void evb::bu::ResourceManager::discardEvent(const EventPtr event)
{
  boost::mutex::scoped_lock sl(allocatedResourcesMutex_);
  
  const AllocatedResources::iterator pos = allocatedResources_.find(event->buResourceId());

  if ( pos == allocatedResources_.end() )
  {
    std::stringstream oss;
    
    oss << "The buResourceId " << event->buResourceId();
    oss << " is not in the allocated resources." ;
    
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  // std::cout << "removing " << event->getEvBid() << " from ";
  // for (EvBids::const_iterator it = pos->second.begin(), itEnd = pos->second.end();
  //      it != itEnd; ++it)
  //   std::cout << *it << " ";
  // std::cout << std::endl;

  pos->second.remove(event->getEvBid());
  --eventMonitoring_.nbEventsInBU;
  
  if ( pos->second.empty() )
  {
    allocatedResources_.erase(pos);
    
    if ( throttle_ )
      while ( ! blockedResourceFIFO_.enq(event->buResourceId()) ) { ::usleep(1000); }
    else
      while ( ! freeResourceFIFO_.enq(event->buResourceId()) ) { ::usleep(1000); }
  } 
}


bool evb::bu::ResourceManager::getResourceId(uint32_t& buResourceId)
{
  if ( freeResourceFIFO_.deq(buResourceId) || ( boost_ && blockedResourceFIFO_.deq(buResourceId) ) )
  {
    boost::mutex::scoped_lock sl(allocatedResourcesMutex_);
    if ( ! allocatedResources_.insert(AllocatedResources::value_type(buResourceId,EvBidList())).second )
    {
      std::stringstream oss;
      
      oss << "The buResourceId " << buResourceId;
      oss << " is already in the allocated resources, while it was also found in the free resources.";
      
      XCEPT_RAISE(exception::EventOrder, oss.str());
    }

    return true;
  }
  
  return false;
}


void evb::bu::ResourceManager::appendMonitoringItems(InfoSpaceItems& items)
{
  nbEventsInBU_ = 0;
  nbEventsBuilt_ = 0;
  rate_ = 0;
  bandwidth_ = 0;
  eventSize_ = 0;
  eventSizeStdDev_ = 0;
  
  items.add("nbEventsInBU", &nbEventsInBU_);
  items.add("nbEventsBuilt", &nbEventsBuilt_);
  items.add("rate", &rate_);
  items.add("bandwidth", &bandwidth_);
  items.add("eventSize", &eventSize_);
  items.add("eventSizeStdDev", &eventSizeStdDev_);
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(eventMonitoringMutex_);
  
  nbEventsInBU_ = eventMonitoring_.nbEventsInBU;
  nbEventsBuilt_ = eventMonitoring_.nbEventsBuilt;
  rate_ = eventMonitoring_.perf.logicalRate();
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
  eventMonitoring_.perf.reset();
}


void evb::bu::ResourceManager::configure()
{
  uint32_t buResourceId;
  while ( freeResourceFIFO_.deq(buResourceId) ) {};
  while ( blockedResourceFIFO_.deq(buResourceId) ) {};
  
  const uint32_t nbResources = std::max(1U,
    bu_->getConfiguration()->maxEvtsUnderConstruction.value_ / 
    bu_->getConfiguration()->eventsPerRequest.value_);
  freeResourceFIFO_.resize(nbResources);
  blockedResourceFIFO_.resize(nbResources);

  for (buResourceId = 0; buResourceId < nbResources; ++buResourceId)
  {
    freeResourceFIFO_.enq(buResourceId);
  }
}


void evb::bu::ResourceManager::clear()
{
  for (AllocatedResources::const_iterator it = allocatedResources_.begin(), itEnd = allocatedResources_.end();
       it != itEnd; ++it)
  {
    freeResourceFIFO_.enq(it->first);
  }
  allocatedResources_.clear();
}


void evb::bu::ResourceManager::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>ResourceManager</p>"                                        << std::endl;
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
    
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->setf(std::ios::fixed);
    out->precision(2);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << bandwidth_ / 1e6 << "</td>"                   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    out->precision(4);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << rate_ << "</td>"                              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>event size (kB)</td>"                              << std::endl;
    *out << "<td>" << eventSize_ / 1e3 <<
      " +/- " << eventSizeStdDev_ / 1e3 << "</td>"                  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  freeResourceFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
    
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  blockedResourceFIFO_.printHtml(out, bu_->getApplicationDescriptor()->getURN());
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
