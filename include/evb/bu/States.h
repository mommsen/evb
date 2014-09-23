#ifndef _evb_bu_States_h_
#define _evb_bu_States_h_

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>

#include "xcept/Exception.h"
#include "xcept/tools.h"

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


    ///////////////////
    // State classes //
    ///////////////////

    /**
     * The outermost state
     */
    class Outermost: public EvBState<Outermost,StateMachine,AllOk>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::in_state_reaction<Release>,
      boost::statechart::in_state_reaction<Throttle>,
      boost::statechart::in_state_reaction<Block>
      > reactions;

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

      typedef boost::mpl::list<
      boost::statechart::transition<Halt,Halted>,
      boost::statechart::in_state_reaction<Fail>
      > reactions;

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

      typedef boost::mpl::list<
      boost::statechart::transition<Fail,Failed,EvBStateMachine,&StateMachine::failEvent>
      > reactions;

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

      typedef boost::mpl::list<
      boost::statechart::transition<Configure,Active>
      > reactions;

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

      typedef boost::mpl::list<
      boost::statechart::transition<Halt,Halted>
      > reactions;

      Active(my_context c) : my_state("Active", c)
      { safeEntryAction(); }
      virtual ~Active()
      { safeExitAction(); }

    };


    /**
     * The Configuring state. Initial state of outer-state Active.
     */
    class Configuring: public EvBState<Configuring,Active>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::transition<ConfigureDone,Ready>
      > reactions;

      Configuring(my_context c) : my_state("Configuring", c)
      { safeEntryAction(); }
      virtual ~Configuring()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      boost::scoped_ptr<boost::thread> configuringThread_;
      volatile bool doConfiguring_;

    };


    /**
     * The Ready state of outer-state Active.
     */
    class Ready: public EvBState<Ready,Active>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::transition<Enable,Running>
      > reactions;

      Ready(my_context c) : my_state("Ready", c)
      { safeEntryAction(); }
      virtual ~Ready()
      { safeExitAction(); }

      virtual void entryAction()
      { outermost_context().notifyRCMS("Ready"); }

    };


    /**
     * The Running state of the outer-state Active.
     */
    class Running: public EvBState<Running,Active,Processing>
    {

    public:

      typedef boost::mpl::list<> reactions;

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
    class Processing: public EvBState<Processing,Running,Enabled>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::transition<Stop,Draining>
      > reactions;

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

      typedef boost::mpl::list<
      boost::statechart::transition<DrainingDone,Ready>
      > reactions;

      Draining(my_context c) : my_state("Draining", c)
      { safeEntryAction(); }
      virtual ~Draining()
      { safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      boost::scoped_ptr<boost::thread> drainingThread_;
      volatile bool doDraining_;

    };


    /**
     * The Enabled state. Initial state of the outer-state Processing.
     */
    class Enabled: public EvBState<Enabled,Processing>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::in_state_reaction<Release>,
      boost::statechart::transition<Throttle,Throttled>,
      boost::statechart::transition<Block,Blocked>
      > reactions;

      Enabled(my_context c) : my_state("Enabled", c)
      { safeEntryAction(); }
      virtual ~Enabled()
      { safeExitAction(); }

      virtual void entryAction()
      { outermost_context().notifyRCMS("Enabled"); }

    };


    /**
     * The Throttled state of the outer-state Processing.
     */
    class Throttled: public EvBState<Throttled,Processing>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::transition<Release,Enabled>,
      boost::statechart::in_state_reaction<Throttle>,
      boost::statechart::transition<Block,Blocked>
      > reactions;

      Throttled(my_context c) : my_state("Throttled", c)
      { safeEntryAction(); }
      virtual ~Throttled()
      { safeExitAction(); }

      virtual void entryAction()
      { outermost_context().notifyRCMS("Throttled"); }

    };


    /**
     * The Blocked state of the outer-state Processing.
     */
    class Blocked: public EvBState<Blocked,Processing>
    {

    public:

      typedef boost::mpl::list<
      boost::statechart::transition<Release,Enabled>,
      boost::statechart::transition<Throttle,Throttled>,
      boost::statechart::in_state_reaction<Block>
      > reactions;

      Blocked(my_context c) : my_state("Blocked", c)
      { safeEntryAction(); }
      virtual ~Blocked()
      { safeExitAction(); }

      virtual void entryAction()
      { outermost_context().notifyRCMS("Blocked"); }

    };

  } } //namespace evb::bu

#endif //_evb_bu_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
