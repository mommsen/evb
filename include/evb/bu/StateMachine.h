#ifndef _evb_bu_StateMachine_h_
#define _evb_bu_StateMachine_h_

#include <memory>

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

    ////////////////////////////////
    // Internal transition events //
    ////////////////////////////////

    class Release: public boost::statechart::event<Release> {};
    class Throttle: public boost::statechart::event<Throttle> {};
    class Block: public boost::statechart::event<Block> {};
    class Misted: public boost::statechart::event<Misted> {};
    class Clouded: public boost::statechart::event<Clouded> {};
    class Pause: public boost::statechart::event<Pause> {};


    ///////////////////////
    // The state machine //
    ///////////////////////

    using EvBStateMachine = EvBStateMachine<StateMachine,Outermost>;
    class StateMachine: public EvBStateMachine
    {

    public:

      StateMachine
      (
        BU*,
        std::shared_ptr<RUproxy>,
        std::shared_ptr<DiskWriter>,
        std::shared_ptr<EventBuilder>,
        std::shared_ptr<ResourceManager>
      );

      BU* bu() const { return bu_; }
      std::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
      std::shared_ptr<DiskWriter> diskWriter() const { return diskWriter_; }
      std::shared_ptr<EventBuilder> eventBuilder() const { return eventBuilder_; }
      std::shared_ptr<ResourceManager> resourceManager() const { return resourceManager_; }

    private:

      BU* bu_;
      std::shared_ptr<RUproxy> ruProxy_;
      std::shared_ptr<DiskWriter> diskWriter_;
      std::shared_ptr<EventBuilder> eventBuilder_;
      std::shared_ptr<ResourceManager> resourceManager_;

    };

  } } //namespace evb::bu

#endif //_evb_bu_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
