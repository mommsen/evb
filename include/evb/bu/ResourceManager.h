#ifndef _evb_bu_ResourceManager_h_
#define _evb_bu_ResourceManager_h_

#include <map>
#include <vector>
#include <stdint.h>

#include <boost/thread/mutex.hpp>

#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/bu/Event.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/Output.h"


namespace evb {

  class BU;
  
  namespace bu { // namespace evb::bu
    
    class StateMachine;
    
    /**
     * \ingroup xdaqApps
     * \brief Manage resources available to the BU
     */
    
    class ResourceManager : public toolbox::lang::Class
    {
      
    public:
      
      ResourceManager(BU*);
      
      virtual ~ResourceManager() {};
      
      /**
       * Mark the resource contained in the passed data block as under construction
       */
      void underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*);

      /**
       * Mark the resouces used by the event as complete
       */
      void eventCompleted(const EventPtr);
      
      /**
       * Discard the event
       */
      void discardEvent(const EventPtr);

      /**
       * Get the next resource id for which to request trigger data.
       * Return false if no free resource id is available.
       */
      bool getResourceId(uint32_t& resourceId);
      
      /**
       * Get the next request for fragments.
       * Return false if no request is available.
       */
      struct Request
      {
        const uint32_t buResourceId;
        const EvBids& evbIds;

        Request(uint32_t buResourceId, const EvBids& evbIds)
        : buResourceId(buResourceId), evbIds(evbIds) {};
      };
      typedef boost::shared_ptr<Request> RequestPtr;
      bool getRequest(RequestPtr&);
      
      /**
       * Append the info space items to be published in the 
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);
      
      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();
      
      /**
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();
      
      /**
       * Configure
       */
      void configure();
      
      /**
       * Remove all data
       */
      void clear();
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      /**
       * Print the content of the request FIFO as HTML snipped
       */
      inline void printRequestFIFO(xgi::Output* out)
      { requestFIFO_.printVerticalHtml(out); }
      
      /**
       * Print the content of the free resource FIFO as HTML snipped
       */
      inline void printFreeResourceFIFO(xgi::Output* out)
      { freeResourceFIFO_.printVerticalHtml(out); }
      
      /**
       * Print the content of the blocked resource FIFO as HTML snipped
       */
      inline void printBlockedResourceFIFO(xgi::Output* out)
      { blockedResourceFIFO_.printVerticalHtml(out); }
      
      
      
    private:
      
      BU* bu_;
      const ConfigurationPtr configuration_;
      
      bool boost_;
      bool throttle_;
      
      typedef std::map<uint32_t, EvBids> AllocatedResources;
      AllocatedResources allocatedResources_;
      boost::mutex allocatedResourcesMutex_;
      
      typedef OneToOneQueue<uint32_t> ResourceFIFO;
      ResourceFIFO freeResourceFIFO_;
      ResourceFIFO blockedResourceFIFO_;
      
      typedef OneToOneQueue<RequestPtr> RequestFIFO;
      RequestFIFO requestFIFO_;
            
      struct EventMonitoring
      {
        uint32_t lastEventNumberFromRUs;
        uint32_t nbEventsBuilt;
        uint32_t nbEventsInBU;
        PerformanceMonitor perf;
      } eventMonitoring_;
      boost::mutex eventMonitoringMutex_;
      
      xdata::UnsignedInteger32 nbEventsInBU_;
      xdata::UnsignedInteger32 nbEventsBuilt_;
      xdata::Double rate_;
      xdata::Double bandwidth_;
      xdata::Double eventSize_;
      xdata::Double eventSizeStdDev_;

    };
    
  } //namespace evb::bu
  
  template <>
  inline void OneToOneQueue<bu::ResourceManager::RequestPtr>::formatter(bu::ResourceManager::RequestPtr request, std::ostringstream* out)
  {
    *out << "Request with buResourceId " << request->buResourceId;
    *out << " for EvBids: " << std::endl;
    for (EvBids::const_iterator it = request->evbIds.begin(), itEnd = request->evbIds.end();
           it != itEnd; ++it)
    {
      try
      {  
        *out << *it << std::endl;
      }
      catch(...)
      {
        *out << "n/a" << std::endl;
      }
    }
  }
  
} //namespace evb

#endif // _evb_bu_ResourceManager_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
