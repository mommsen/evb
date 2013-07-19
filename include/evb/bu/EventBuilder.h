#ifndef _evb_bu_EventBuilder_h_
#define _evb_bu_EventBuilder_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>
#include <vector>

#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/Event.h"
#include "evb/bu/RUproxy.h"
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
        boost::shared_ptr<RUproxy>,
        boost::shared_ptr<DiskWriter>,
        boost::shared_ptr<ResourceManager>
      );

      /**
       * Add the super fragment received for the BU resource id
       */
      void addSuperFragment(const uint32_t buResourceId, FragmentChainPtr&);

      /**
       * Configure
       */
      void configure();

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
      void startProcessing(const uint32_t runNumber);

      /**
       * Stop processing messages
       */
      void stopProcessing();

      /**
       * Print the content of the super-fragment FIFOs as HTML snipped
       */
      void printSuperFragmentFIFOs(xgi::Output*);


    private:

      // Lookup table of events, indexed by evb id
      typedef std::map<EvBid,EventPtr> EventMap;
      typedef boost::shared_ptr<EventMap> EventMapPtr;
      typedef std::map<uint16_t,EventMapPtr> EventMaps;
      EventMaps eventMaps_;

      void createProcessingWorkLoops();
      bool process(toolbox::task::WorkLoop*);
      void buildEvent(FragmentChainPtr&, EventMapPtr&);
      EventMap::iterator getEventPos(EventMapPtr&, const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*, const uint16_t superFragmentCount);

      BU* bu_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<DiskWriter> diskWriter_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;

      const ConfigurationPtr configuration_;
      uint32_t runNumber_;

      typedef OneToOneQueue<FragmentChainPtr> SuperFragmentFIFO;
      typedef boost::shared_ptr<SuperFragmentFIFO> SuperFragmentFIFOPtr;
      typedef std::map<uint16_t,SuperFragmentFIFOPtr> SuperFragmentFIFOs;
      SuperFragmentFIFOs superFragmentFIFOs_;

      typedef std::vector<toolbox::task::WorkLoop*> WorkLoops;
      WorkLoops builderWorkLoops_;
      toolbox::task::ActionSignature* builderAction_;

      volatile bool doProcessing_;
      volatile bool processActive_;

    }; // EventBuilder

    typedef boost::shared_ptr<EventBuilder> EventBuilderPtr;

  } } // namespace evb::bu

#endif // _evb_bu_EventBuilder_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
