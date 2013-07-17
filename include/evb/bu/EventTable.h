#ifndef _evb_bu_EventTable_h_
#define _evb_bu_EventTable_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/Event.h"
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
    class ResourceManager;
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
        boost::shared_ptr<DiskWriter>,
        boost::shared_ptr<ResourceManager>
      );

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


    private:

      // Lookup table of events, indexed by evb id
      typedef std::map<EvBid,EventPtr> EventMap;
      EventMap eventMap_;

      void startProcessingWorkLoop();
      bool process(toolbox::task::WorkLoop*);
      bool buildEvents();
      EventMap::iterator getEventPos(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*, const uint16_t superFragmentCount);

      BU* bu_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<DiskWriter> diskWriter_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;

      uint32_t runNumber_;

      toolbox::task::WorkLoop* processingWL_;
      toolbox::task::ActionSignature* processingAction_;

      volatile bool doProcessing_;
      volatile bool processActive_;

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
