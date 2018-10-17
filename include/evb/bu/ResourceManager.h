#ifndef _evb_bu_ResourceManager_h_
#define _evb_bu_ResourceManager_h_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <stdint.h>
#include <string>
#include <time.h>

#include <boost/filesystem/convenience.hpp>

#include "cgicc/HTMLClasses.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/Event.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdata/Double.h"
#include "xdata/Integer32.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb {

  class BU;

  namespace bu { // namespace evb::bu

    /**
     * \ingroup xdaqApps
     * \brief Manage resources available to the BU
     */

    class ResourceManager : public toolbox::lang::Class
    {

    public:

      ResourceManager(BU*);

      ~ResourceManager();

      /**
       * Mark the resource contained in the passed data block as under construction.
       * Returns the builder identifier to be used for building this resource.
       */
      uint16_t underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*);

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
      struct BUresource
      {
        uint16_t id;
        uint16_t priority;
        uint16_t eventsToDiscard;

        BUresource(const uint16_t id,const uint16_t priority,const uint16_t eventsToDiscard)
          : id(id),priority(priority),eventsToDiscard(eventsToDiscard) {};
      };
      using BUresources = std::vector<BUresource>;
      void getAllAvailableResources(BUresources&);

      /**
       * Return the next lumi-section account.
       * Return false is no lumi-section account is available.
       */
      struct LumiSectionAccount
      {
        const uint32_t lumiSection;
        time_t startTime;
        uint32_t nbEvents;
        uint32_t nbIncompleteEvents;

        LumiSectionAccount(const uint32_t ls)
          : lumiSection(ls),startTime(time(0)),nbEvents(0),nbIncompleteEvents(0) {};
      };
      using LumiSectionAccountPtr = std::shared_ptr<LumiSectionAccount>;

      bool getNextLumiSectionAccount(LumiSectionAccountPtr&,const bool completeLumiSectionsOnly);

      /**
       * Return oldest lumi section number for which events are being built
       */
      uint32_t getOldestIncompleteLumiSection() const;

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
       * Configure
       */
      void configure();

      /**
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;

      /**
       * Return the resource table as cgicc snipped
       */
      cgicc::div getHtmlSnippedForResourceTable() const;

      /**
       * Return the averaged event size
       */
      uint32_t getEventSize() const;

      /**
       * Return the averaged event rate
       */
      uint32_t getEventRate() const;

      /**
       * Return the averaged throughput into the BU
       */
      uint32_t getThroughput() const;

      /**
       * Return the current number of events in the BU
       */
      uint32_t getNbEventsInBU() const;

      /**
       * Return the number of events built since the start of the run
       */
      uint64_t getNbEventsBuilt() const;

      /**
       * Enable or disable the requests for events
       */
      void requestEvents(const bool val)
      { requestEvents_ = val; }

    private:

      void resetMonitoringCounters();
      void incrementEventsInLumiSection(const uint32_t lumiSection);
      void eventCompletedForLumiSection(const uint32_t lumiSection);
      void configureResources();
      void configureResourceSummary();
      void configureDiskUsageMonitors();
      float getAvailableResources(std::string& statusMsg, std::string& statusKeys);
      float getOverThreshold();
      void updateDiskUsages();
      void handleResourceSummaryFailure(const std::string& msg);
      void startResourceMonitorWorkLoop();
      bool resourceMonitor(toolbox::task::WorkLoop*);
      void updateResources(const float availableResources, std::string& statusMsg, std::string& statusKeys);
      uint16_t getPriority();
      void changeStatesBasedOnResources();

      BU* bu_;
      const ConfigurationPtr configuration_;
      uint32_t lumiSectionTimeout_;

      using EvBidList = std::list<EvBid>;
      struct ResourceInfo
      {
        int16_t builderId;
        bool blocked;
        EvBidList evbIdList;
      };
      using BuilderResources = std::map<uint16_t,ResourceInfo>;
      BuilderResources builderResources_;
      mutable std::mutex builderResourcesMutex_;

      using ResourceFIFO = OneToOneQueue<BuilderResources::iterator>;
      ResourceFIFO resourceFIFO_;
      mutable std::mutex resourceFIFOmutex_;

      uint32_t runNumber_;
      uint32_t eventsToDiscard_;
      uint32_t nbResources_;
      uint32_t blockedResources_;
      uint16_t currentPriority_;
      uint16_t builderId_;
      uint32_t fusHLT_;
      uint32_t fusCloud_;
      uint32_t fusQuarantined_;
      uint32_t fusStale_;
      uint32_t allFUsStaleDetected_;
      uint32_t initiallyQueuedLS_;
      uint32_t queuedLS_;
      int32_t queuedLSonFUs_;
      double fuOutBwMB_;
      bool requestEvents_;
      bool pauseRequested_;
      mutable std::mutex lsLatencyMutex_;

      boost::filesystem::path resourceSummary_;
      mutable std::mutex resourceSummaryMutex_;
      bool resourceSummaryFailureAlreadyNotified_;
      bool resourceLimitiationAlreadyNotified_;

      using DiskUsageMonitors = std::vector<DiskUsagePtr>;
      DiskUsageMonitors diskUsageMonitors_;
      mutable std::mutex diskUsageMonitorsMutex_;

      using LumiSectionAccounts = std::map<uint32_t,LumiSectionAccountPtr>;
      LumiSectionAccounts lumiSectionAccounts_;
      mutable std::mutex lumiSectionAccountsMutex_;
      uint32_t oldestIncompleteLumiSection_;
      volatile bool doProcessing_;
      toolbox::task::WorkLoop* resourceMonitorWL_;

      struct EventMonitoring
      {
        uint64_t nbEventsBuilt;
        uint32_t nbEventsInBU;
        uint32_t eventSize;
        uint32_t eventSizeStdDev;
        int32_t outstandingRequests;
        PerformanceMonitor perf;
      } eventMonitoring_;
      mutable std::mutex eventMonitoringMutex_;

      std::string statusMessage_;
      std::string statusKeys_;
      mutable std::mutex statusMessageMutex_;

      xdata::UnsignedInteger32 nbEventsInBU_;
      xdata::UnsignedInteger64 nbEventsBuilt_;
      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger64 throughput_;
      xdata::UnsignedInteger32 eventSize_;
      xdata::UnsignedInteger32 eventSizeStdDev_;
      xdata::UnsignedInteger32 nbTotalResources_;
      xdata::UnsignedInteger32 nbFreeResources_;
      xdata::UnsignedInteger32 nbSentResources_;
      xdata::UnsignedInteger32 nbUsedResources_;
      xdata::UnsignedInteger32 nbBlockedResources_;
      xdata::UnsignedInteger32 priority_;
      xdata::UnsignedInteger32 fuSlotsHLT_;
      xdata::UnsignedInteger32 fuSlotsCloud_;
      xdata::UnsignedInteger32 fuSlotsQuarantined_;
      xdata::UnsignedInteger32 fuSlotsStale_;
      xdata::Double fuOutputBandwidthInMB_;
      xdata::Integer32 queuedLumiSections_;
      xdata::Integer32 queuedLumiSectionsOnFUs_;
      xdata::Double ramDiskSizeInGB_;
      xdata::Double ramDiskUsed_;
      xdata::String statusMsg_;
      xdata::String statusKeywords_;

      friend std::ostream& operator<<(std::ostream&,const BuilderResources::const_iterator);
    };

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

    inline std::ostream& operator<<
    (
      std::ostream& s,
      const evb::bu::ResourceManager::BuilderResources::const_iterator it
    )
    {
      s << "resourceId=" << it->first << " ";
      if ( it->second.blocked )
      {
        s << "BLOCKED";
      }
      else if ( it->second.builderId == -1 )
      {
        s << "free";
      }
      else
      {
        s << "builderId=" << it->second.builderId << " ";
        for (evb::bu::ResourceManager::EvBidList::const_iterator evbIt =
               it->second.evbIdList.begin(), evbItEnd = it->second.evbIdList.end();
             evbIt != evbItEnd; ++evbIt)
        {
          s << *evbIt << " ";
        }
      }
      return s;
    }

  } } //namespace evb::bu


#endif // _evb_bu_ResourceManager_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
