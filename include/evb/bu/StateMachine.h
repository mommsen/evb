#ifndef _evb_bu_StateMachine_h_
#define _evb_bu_StateMachine_h_

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/Exception.h"
#include "evb/EvBStateMachine.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class BU;

  namespace bu {

    class FUproxy;
    class RUproxy;
    class DiskWriter;
    class EventBuilder;
    class ResourceManager;
    class StateMachine;
    class Outermost;

    ///////////////////////
    // The state machine //
    ///////////////////////

    typedef EvBStateMachine<StateMachine,Outermost> EvBStateMachine;
    class StateMachine: public EvBStateMachine
    {

    public:

      StateMachine
      (
        BU*,
        boost::shared_ptr<RUproxy>,
        boost::shared_ptr<DiskWriter>,
        boost::shared_ptr<EventBuilder>,
        boost::shared_ptr<ResourceManager>
      );

      BU* bu() const { return bu_; }
      boost::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
      boost::shared_ptr<DiskWriter> diskWriter() const { return diskWriter_; }
      boost::shared_ptr<EventBuilder> eventBuilder() const { return eventBuilder_; }
      boost::shared_ptr<ResourceManager> resourceManager() const { return resourceManager_; }

    private:

      BU* bu_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<DiskWriter> diskWriter_;
      boost::shared_ptr<EventBuilder> eventBuilder_;
      boost::shared_ptr<ResourceManager> resourceManager_;

    };

  } } //namespace evb::bu

#endif //_evb_bu_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
