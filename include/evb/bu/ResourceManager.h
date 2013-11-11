#ifndef _evb_bu_ResourceManager_h_
#define _evb_bu_ResourceManager_h_

#include <map>
#include <vector>
#include <stdint.h>
#include <time.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/Event.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
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
      void underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*&);

      /**
       * Mark the resouces used by the event as complete
       */
      void eventCompleted(const EventPtr&);

      /**
       * Discard the event
       */
      void discardEvent(const EventPtr&);

      /**
       * Get the next resource id for which to request trigger data.
       * Return false if no free resource id is available.
       */
      bool getResourceId(uint32_t& resourceId);

      /**
       * Return the next lumi-section account.
       * Return false is no lumi-section account is available.
       */
      struct LumiSectionAccount
      {
        const time_t startTime;
        const uint32_t lumiSection;
        uint32_t nbEvents;

        LumiSectionAccount(const uint32_t ls)
        : startTime(time(0)),lumiSection(ls),nbEvents(0) {};
      };
      typedef boost::shared_ptr<LumiSectionAccount> LumiSectionAccountPtr;

      bool getNextLumiSectionAccount(LumiSectionAccountPtr&);

      /**
       * Start processing messages
       */
      void startProcessing();

      /**
       * Drain messages
       */
      void drain();

      /**
       * Stop processing messages
       */
      void stopProcessing();

      /**
       * Add the DiskUsagePtr to the disks to be monitored
       */
      void monitorDiskUsage(DiskUsagePtr&);

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
       * Print monitoring information as HTML snipped
       */
      void printHtml(xgi::Output*) const;

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

      /**
       * Print the content of the lumi-section account FIFO as HTML snipped
       */
      inline void printLumiSectionAccountFIFO(xgi::Output* out)
      { lumiSectionAccountFIFO_.printVerticalHtml(out); }


    private:

      void incrementEventsInLumiSection(const uint32_t lumiSection);
      void enqCurrentLumiSectionAccount();
      void getDiskUsages();

      BU* bu_;
      const ConfigurationPtr configuration_;
      uint32_t lumiSectionTimeout_;

      int16_t throttleResources_;

      typedef std::list<EvBid> EvBidList;
      typedef std::map<uint32_t, EvBidList> AllocatedResources;
      AllocatedResources allocatedResources_;
      boost::mutex allocatedResourcesMutex_;

      uint32_t nbResources_;
      typedef OneToOneQueue<uint32_t> ResourceFIFO;
      ResourceFIFO freeResourceFIFO_;
      ResourceFIFO blockedResourceFIFO_;

      typedef std::vector<DiskUsagePtr> DiskUsageMonitors;
      DiskUsageMonitors diskUsageMonitors_;

      typedef OneToOneQueue<LumiSectionAccountPtr> LumiSectionAccountFIFO;
      LumiSectionAccountFIFO lumiSectionAccountFIFO_;
      LumiSectionAccountPtr currentLumiSectionAccount_;
      boost::mutex currentLumiSectionAccountMutex_;

      struct EventMonitoring
      {
        uint32_t nbEventsBuilt;
        uint32_t nbEventsInBU;
        uint32_t outstandingRequests;
        PerformanceMonitor perf;
      } eventMonitoring_;
      mutable boost::mutex eventMonitoringMutex_;

      xdata::UnsignedInteger32 nbEventsInBU_;
      xdata::UnsignedInteger32 nbEventsBuilt_;
      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger64 bandwidth_;
      xdata::UnsignedInteger32 eventSize_;
      xdata::UnsignedInteger32 eventSizeStdDev_;

    };

  } } //namespace evb::bu


inline std::ostream& operator<<
(
  std::ostream& s,
  const evb::bu::ResourceManager::LumiSectionAccountPtr lumiAccount
)
{
  if ( lumiAccount.get() )
  {
    s << "lumiSection=" << lumiAccount->lumiSection << " ";
    s << "nbEvents=" << lumiAccount->nbEvents;
  }

  return s;
}


#endif // _evb_bu_ResourceManager_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
