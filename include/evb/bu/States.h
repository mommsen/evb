#ifndef _evb_bu_States_h_
#define _evb_bu_States_h_

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/deep_history.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>
#include <boost/thread/thread.hpp>

#include "xcept/Exception.h"
#include "xcept/tools.h"

#include <memory>
#include <string>


namespace evb {

  namespace bu {


    ///////////////////////////////////////////
    // Forward declarations of state classes //
    ///////////////////////////////////////////

    class Outermost;
    // Outer states:
    class Failed;
    class AllOk;
    // Inner states of AllOk
    class Halted;
    class Active;
    // Inner states of Active
    class Configuring;
    class Ready;
    class Running;
    // Inner states of Running
    class Processing;
    class Draining;
    // Inner states of Processing
    class Enabled;
    class Throttled;
    class Blocked;
    class Mist;
    class Cloud;
    class Paused;


    ///////////////////
    // State classes //
    ///////////////////

    /**
     * The outermost state
     */
    class Outermost: public EvBState<Outermost,StateMachine,AllOk>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Release>,
      boost::statechart::in_state_reaction<Throttle>,
      boost::statechart::in_state_reaction<Block>,
      boost::statechart::in_state_reaction<Misted>,
      boost::statechart::in_state_reaction<Clouded>,
      boost::statechart::in_state_reaction<Pause>
      >;

      Outermost(my_context c) : my_state("Outermost", c)
      { safeEntryAction(); }
      virtual ~Outermost()
      { safeExitAction(); }

    };


    /**
     * Failed state
     */
    class Failed: public EvBState<Failed,Outermost>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Halt,Halted>,
      boost::statechart::in_state_reaction<Fail>
      >;

      Failed(my_context c) : my_state("Failed", c)
      { safeEntryAction(); }
      virtual ~Failed()
      { safeExitAction(); }

      virtual void exitAction()
      { this->outermost_context().clearError(); }

    };

    /**
     * The default state AllOk. Initial state of outer-state Outermost
     */
    class AllOk: public EvBState<AllOk,Outermost,Halted>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Fail,Failed,EvBStateMachine,&StateMachine::failEvent>
      >;

      AllOk(my_context c) : my_state("AllOk", c)
      { safeEntryAction(); }
      virtual ~AllOk()
      { safeExitAction(); }

    };


    /**
     * The Halted state. Initial state of outer-state AllOk.
     */
    class Halted: public EvBState<Halted,AllOk>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Configure,Active>,
      boost::statechart::in_state_reaction<Halt>
      >;

      Halted(my_context c) : my_state("Halted", c)
      { safeEntryAction(); }
      virtual ~Halted()
      { safeExitAction(); }

    };


    /**
     * The Active state of outer-state AllOk.
     */
    class Active: public EvBState<Active,AllOk,Configuring>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Halt,Halted>
      >;

      Active(my_context c) : my_state("Active", c)
      { safeEntryAction(); }
      virtual ~Active()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    };


    /**
     * The Configuring state. Initial state of outer-state Active.
     */
    class Configuring: public EvBState<Configuring,Active>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<ConfigureDone,Ready>
      >;

      Configuring(my_context c) : my_state("Configuring", c)
      { safeEntryAction(); }
      virtual ~Configuring()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      std::unique_ptr<boost::thread> configuringThread_;
      volatile bool doConfiguring_;

    };


    /**
     * The Ready state of outer-state Active.
     */
    class Ready: public EvBState<Ready,Active>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Enable,Running>,
      boost::statechart::in_state_reaction<Clear>
      >;

      Ready(my_context c) : my_state("Ready", c)
      { safeEntryAction(); }
      virtual ~Ready()
      { safeExitAction(); }

      virtual void entryAction();

    };


    /**
     * The Running state of the outer-state Active.
     */
    class Running: public EvBState<Running,Active,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Clear,Configuring>
      >;

      Running(my_context c) : my_state("Running", c)
      { safeEntryAction(); }
      virtual ~Running()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    };


    /**
     * The Processing state. Initial state of the outer-state Running.
     */
    class Processing: public EvBState<Processing,Running,
                                      boost::mpl::list< boost::statechart::deep_history<Enabled> >,
                                      boost::statechart::has_deep_history>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<Stop,Draining>,
      boost::statechart::transition<Release,Enabled>,
      boost::statechart::transition<Throttle,Throttled>,
      boost::statechart::transition<Block,Blocked>,
      boost::statechart::transition<Misted,Mist>,
      boost::statechart::transition<Clouded,Cloud>,
      boost::statechart::transition<Pause,Paused>
      >;

      Processing(my_context c) : my_state("Processing", c)
      { safeEntryAction(); }
      virtual ~Processing()
      { safeExitAction(); }

    };


    /**
     * The Draining state of outer-state Running.
     */
    class Draining: public EvBState<Draining,Running>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::transition<DrainingDone,Ready>
      >;

      Draining(my_context c) : my_state("Draining", c)
      { safeEntryAction(); }
      virtual ~Draining()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      std::unique_ptr<boost::thread> drainingThread_;
      volatile bool doDraining_;

    };


    /**
     * The Enabled state. Initial state of the outer-state Processing.
     */
    class Enabled: public EvBState<Enabled,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Release>
      >;

      Enabled(my_context c) : my_state("Enabled", c)
      { safeEntryAction(); }
      virtual ~Enabled()
      { safeExitAction(); }

    };


    /**
     * The Throttled state of the outer-state Processing.
     */
    class Throttled: public EvBState<Throttled,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Throttle>
      >;

      Throttled(my_context c) : my_state("Throttled", c)
      { safeEntryAction(); }
      virtual ~Throttled()
      { safeExitAction(); }

    };


    /**
     * The Blocked state of the outer-state Processing.
     */
    class Blocked: public EvBState<Blocked,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Block>
      >;

      Blocked(my_context c) : my_state("Blocked", c)
      { safeEntryAction(); }
      virtual ~Blocked()
      { safeExitAction(); }

    };


    /**
     * The Mist state of the outer-state Processing.
     */
    class Mist: public EvBState<Mist,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Misted>
      >;

      Mist(my_context c) : my_state("Mist", c)
      { safeEntryAction(); }
      virtual ~Mist()
      { safeExitAction(); }

    };


    /**
     * The Cloud state of the outer-state Processing.
     */
    class Cloud: public EvBState<Cloud,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Clouded>
      >;

      Cloud(my_context c) : my_state("Cloud", c)
      { safeEntryAction(); }
      virtual ~Cloud()
      { safeExitAction(); }

    };


    /**
     * The Paused state of the outer-state Processing.
     */
    class Paused: public EvBState<Paused,Processing>
    {

    public:

      using reactions = boost::mpl::list<
      boost::statechart::in_state_reaction<Paused>
      >;

      Paused(my_context c) : my_state("Paused", c)
      { safeEntryAction(); }
      virtual ~Paused()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    };

  } } //namespace evb::bu

#endif //_evb_bu_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
