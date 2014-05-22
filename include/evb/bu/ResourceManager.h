#ifndef _evb_bu_ResourceManager_h_
#define _evb_bu_ResourceManager_h_

#include <map>
#include <vector>
#include <stdint.h>
#include <time.h>

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "cgicc/HTMLClasses.h"
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
      bool getResourceId(uint16_t& resourceId, uint16_t& eventsToDiscard);

      /**
       * Return the next lumi-section account.
       * Return false is no lumi-section account is available.
       */
      struct LumiSectionAccount
      {
        const uint32_t lumiSection;
        time_t startTime;
        uint32_t nbEvents;

        LumiSectionAccount(const uint32_t ls)
          : lumiSection(ls),startTime(time(0)),nbEvents(0) {};
      };
      typedef boost::shared_ptr<LumiSectionAccount> LumiSectionAccountPtr;

      bool getNextLumiSectionAccount(LumiSectionAccountPtr&);

      /**
       * Return current lumi section number
       */
      uint32_t getCurrentLumiSection();

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
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;


    private:

      void incrementEventsInLumiSection(const uint32_t lumiSection);
      void enqCurrentLumiSectionAccount();
      void configureDiskUsageMonitors();
      float getAvailableResources();
      void updateResources();

      BU* bu_;
      const ConfigurationPtr configuration_;
      uint32_t lumiSectionTimeout_;

      typedef std::list<EvBid> EvBidList;
      typedef std::map<uint16_t, EvBidList> AllocatedResources;
      AllocatedResources allocatedResources_;
      boost::mutex allocatedResourcesMutex_;
      uint32_t eventsToDiscard_;

      uint32_t nbResources_;
      uint32_t resourcesToBlock_;
      typedef OneToOneQueue<uint16_t> ResourceFIFO;
      ResourceFIFO freeResourceFIFO_;
      ResourceFIFO blockedResourceFIFO_;
      boost::filesystem::path resourceDirectory_;

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
      xdata::UnsignedInteger32 fuCoresAvailable_;

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
