#ifndef _evb_bu_EventTable_h_
#define _evb_bu_EventTable_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/Event.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
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
    class RUproxy;
    class StateMachine;
    
    /**
     * \ingroup xdaqApps
     * \brief Keep track of events
     */
    
    class EventTable : public toolbox::lang::Class
    {
    public:
      
      EventTable
      (
        BU*,
        boost::shared_ptr<RUproxy>,
        boost::shared_ptr<DiskWriter>
      );
      
      /**
       * Start construction of a new event.
       * The first event fragment must be the trigger block
       * received from the EVM. The ruCount specifies
       * the number of RUs supposed to send a super fragment
       * to make a complete event.
       */
      void startConstruction(const uint32_t ruCount, toolbox::mem::Reference*);
      
      /**
       * Appand a super fragment to an event under construction
       */
      void appendSuperFragment(toolbox::mem::Reference*);
      
      /**
       * Append the info space parameters used for the
       * configuration to the InfoSpaceItems
       */
      void appendConfigurationItems(InfoSpaceItems&);
      
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
      void configure(const uint32_t maxEvtsUnderConstruction);
      
      /**
       * Remove all data
       */
      void clear();
      
      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }
      
      /**
       * Start processing messages
       */
      void startProcessing();

      /**
       * Stop processing messages
       */
      void stopProcessing();
      
      /**
       * Allow or disallow requests for new events
       */
      void requestEvents(bool val)
      { requestEvents_ = val; }
      
      /**
       * Print the monitoring information as HTML snipped
       */
      void printMonitoringInformation(xgi::Output*);
      
      /**
       * Print the block FIFO as HTML snipped
       */
      inline void printBlockFIFO(xgi::Output* out)
      { blockFIFO_.printVerticalHtml(out); }
      
      /**
       * Print the configuration information as HTML snipped
       */
      inline void printConfiguration(xgi::Output* out)
      { tableParams_.printHtml("Configuration", out); }
      
      
    private:
      
      void checkForCompleteEvent(EventPtr);
      void updateEventCounters(EventPtr);
      void startProcessingWorkLoop();
      bool process(toolbox::task::WorkLoop*);
      bool assembleDataBlockMessages();
      bool buildEvents();
      bool handleCompleteEvents();
      
      BU* bu_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<DiskWriter> diskWriter_;
      boost::shared_ptr<StateMachine> stateMachine_;
      
      // Lookup table of data blocks, indexed by RU tid and BU resource id
      struct Index
      {
        uint32_t ruTid;
        uint32_t resourceId;
        
        inline bool operator< (const Index& other) const
        { return ruTid == other.ruTid ? resourceId < other.resourceId : ruTid < other.ruTid; }
      };
      typedef std::map<Index,FragmentChainPtr> DataBlockMap;
      DataBlockMap dataBlockMap_;

      // Lookup table of events, indexed by evb id
      typedef std::map<EvBid,EventPtr> EventMap;
      EventMap eventMap_;
      FragmentChain::ResourceList ruTids_;
      
      typedef OneToOneQueue<toolbox::mem::Reference*> BlockFIFO;
      BlockFIFO blockFIFO_;
      
      toolbox::task::WorkLoop* processingWL_;
      toolbox::task::ActionSignature* processingAction_;
      
      volatile bool doProcessing_;
      volatile bool processActive_;
      volatile bool requestEvents_;
      
      InfoSpaceItems tableParams_;
      xdata::Boolean dropEventData_;
      
      struct EventMonitoring
      {
        uint32_t nbEventsUnderConstruction;
        uint32_t nbEventsInBU;
        uint32_t nbEventsDropped;
        PerformanceMonitor perf;
      } eventMonitoring_;
      boost::mutex eventMonitoringMutex_;
      
      xdata::UnsignedInteger32 nbEvtsUnderConstruction_;
      xdata::UnsignedInteger32 nbEvtsReady_;
      xdata::UnsignedInteger32 nbEventsInBU_;
      xdata::UnsignedInteger32 nbEvtsBuilt_;
      xdata::Double rate_;
      xdata::Double bandwidth_;
      xdata::Double eventSize_;
      xdata::Double eventSizeStdDev_;
      
    }; // EventTable
    
    typedef boost::shared_ptr<EventTable> EventTablePtr;
    
  } } // namespace evb::bu

#endif // _evb_bu_EventTable_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
