#ifndef _evb_bu_EventBuilder_h_
#define _evb_bu_EventBuilder_h_

#include <boost/dynamic_bitset.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <memory>
#include <stdint.h>
#include <vector>

#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/Event.h"
#include "evb/bu/FragmentChain.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/StreamHandler.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class BU;

  namespace bu {

    class DiskWriter;
    class ResourceManager;
    class StateMachine;

    /**
     * \ingroup xdaqApps
     * \brief Keep track of events
     */

    class EventBuilder : public toolbox::lang::Class
    {
    public:

      EventBuilder
      (
        BU*,
        std::shared_ptr<DiskWriter>,
        std::shared_ptr<ResourceManager>
      );

      ~EventBuilder();

      /**
       * Add the super fragment received for the BU resource id
       */
      void addSuperFragment(const uint16_t buResourceId, FragmentChainPtr&);

      /**
       * Configure
       */
      void configure();

      /**
       * Register the state machine
       */
      void registerStateMachine(std::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }

      /**
       * Start processing events
       */
      void startProcessing(const uint32_t runNumber);

      /**
       * Drain events
       */
      void drain() const;

      /**
       * Stop processing events
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
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;

      /**
       * Write the next count events to a text file
       */
      void writeNextEventsToFile(const uint16_t count);

      /**
       * Return the number of corrupted events since the start of the run
       */
      uint64_t getNbCorruptedEvents() const;

      /**
       * Return the number of events with CRC errors since the start of the run
       */
      uint64_t getNbEventsWithCRCerrors() const;

      /**
       * Return the number of events with missing FED data since the start of the run
       */
      uint64_t getNbEventsMissingData() const;


    private:

      using PartialEvents = std::map<EvBid,EventPtr>;           //indexed by EvBid
      using CompleteEvents = std::multimap<uint32_t,EventPtr>;  //indexed by lumi section

      struct EventMapMonitor
      {
        uint32_t lowestLumiSection;
        uint32_t completeEvents;
        uint32_t partialEvents;

        EventMapMonitor() :
          lowestLumiSection(0),completeEvents(0),partialEvents(0) {};

        void reset()
        { lowestLumiSection = 0; completeEvents = 0; partialEvents = 0; }
      };

      void createProcessingWorkLoops();
      bool process(toolbox::task::WorkLoop*);
      void buildEvent(FragmentChainPtr&, PartialEvents&, CompleteEvents&) const;
      PartialEvents::iterator getEventPos(PartialEvents&, const EvBid&, const msg::RUtids&, const uint16_t& buResourceId) const;
      uint32_t handleCompleteEvents(CompleteEvents&, StreamHandlerPtr&) const;
      bool isEmpty() const;

      BU* bu_;
      std::shared_ptr<DiskWriter> diskWriter_;
      std::shared_ptr<ResourceManager> resourceManager_;
      std::shared_ptr<StateMachine> stateMachine_;

      const ConfigurationPtr configuration_;
      uint32_t runNumber_;

      using SuperFragmentFIFO = OneToOneQueue<FragmentChainPtr>;
      using SuperFragmentFIFOPtr = std::shared_ptr<SuperFragmentFIFO>;
      using SuperFragmentFIFOs = std::map<uint16_t,SuperFragmentFIFOPtr>;
      SuperFragmentFIFOs superFragmentFIFOs_;

      using WorkLoops = std::vector<toolbox::task::WorkLoop*>;
      WorkLoops builderWorkLoops_;
      toolbox::task::ActionSignature* builderAction_;

      using EventMapMonitors = std::map<uint16_t,EventMapMonitor>;
      EventMapMonitors eventMapMonitors_;

      uint64_t corruptedEvents_;
      uint64_t eventsWithCRCerrors_;
      uint64_t eventsMissingData_;
      mutable boost::mutex errorCountMutex_;

      volatile bool doProcessing_;
      boost::dynamic_bitset<> processesActive_;
      mutable boost::mutex processesActiveMutex_;

      mutable uint16_t writeNextEventsToFile_;
      mutable boost::mutex writeNextEventsToFileMutex_;

      xdata::UnsignedInteger64 nbCorruptedEvents_;
      xdata::UnsignedInteger64 nbEventsWithCRCerrors_;
      xdata::UnsignedInteger64 nbEventsMissingData_;

    }; // EventBuilder

    using EventBuilderPtr = std::shared_ptr<EventBuilder>;

  } } // namespace evb::bu

#endif // _evb_bu_EventBuilder_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
