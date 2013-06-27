#ifndef _evb_readoutunit_States_h_
#define _evb_readoutunit_States_h_

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>

#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"

#include <string>


namespace evb {
  
  namespace readoutunit {

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
    class Configured;
    class Processing;
    // Inner states of Configured
    class Clearing;
    class Ready;
    // Inner states of Processing
    class Enabled;
    // Inner states of Enabled
    class MismatchDetectedBackPressuring;
    class TimedOutBackPressuring;
    
    typedef StateMachine< ReadoutUnit< Configuration,Input<Configuration> > > ReadoutUnitStateMachine;
    
    
    ///////////////////
    // State classes //
    ///////////////////
    
    /**
     * The outermost state
     */
    class Outermost: public EvBState<Outermost,ReadoutUnitStateMachine,AllOk>
    {
      
    public:
      
      typedef boost::mpl::list<> reactions;
      
      Outermost(my_context c) : my_state("Outermost", c)
      { 
        std::cout << "Outermost" << std::endl;
        safeEntryAction();
      }
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
      boost::statechart::transition<Fail,Failed>
      > reactions;
      
      Failed(my_context c) : my_state("Failed", c)
      { safeEntryAction(); }
      virtual ~Failed()
      { safeExitAction(); }
      
    };
    
    /**
     * The default state AllOk. Initial state of outer-state Outermost
     */
    class AllOk: public EvBState<AllOk,Outermost,Halted>
    {
      
    public:
      
      typedef boost::mpl::list<
      boost::statechart::transition<Fail,Failed,EvBStateMachine<ReadoutUnitStateMachine,Outermost>,&ReadoutUnitStateMachine::failEvent>
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
    boost::statechart::transition<ConfigureDone,Configured>
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
     * The Configured state of the outer-state Active.
     */
    class Configured: public EvBState<Configured,Active,Clearing>
    {
      
    public:
      
      Configured(my_context c) : my_state("Configured", c)
      { safeEntryAction(); }
      virtual ~Configured()
      { safeExitAction(); }
      
    };
    
    
    /**
     * The Processing state of the outer-state Active.
     */
    class Processing: public EvBState<Processing,Active,Enabled>
    {
      
    public:
      
      typedef boost::mpl::list<
      boost::statechart::transition<Stop,Configured>
      > reactions;
      
      Processing(my_context c) : my_state("Processing", c)
      { safeEntryAction(); }
      virtual ~Processing()
      { safeExitAction(); }
      
      virtual void entryAction();

    };
    
    
    /**
     * The Clearing state. Initial state of outer-state Configured.
     */
    class Clearing: public EvBState<Clearing,Configured>
    {
      
    public:
      
      typedef boost::mpl::list<
      boost::statechart::transition<ClearDone,Ready>
      > reactions;
      
      Clearing(my_context c) : my_state("Clearing", c)
      { safeEntryAction(); }
      virtual ~Clearing()
      { safeExitAction(); }
      
      virtual void entryAction();
      virtual void exitAction();
      void activity();
      
    private:
      boost::scoped_ptr<boost::thread> clearingThread_;
      volatile bool doClearing_;
      
    };
    
    
    /**
     * The Ready state of outer-state Configured.
     */
    class Ready: public EvBState<Ready,Configured>
    {
      
    public:
      
      typedef boost::mpl::list<
      boost::statechart::transition<Enable,Processing>
      > reactions;
      
      Ready(my_context c) : my_state("Ready", c)
      { safeEntryAction(); }
      virtual ~Ready()
      { safeExitAction(); }
      
      virtual void entryAction()
      { outermost_context().notifyRCMS("Ready"); }
      
    };
    
    
    /**
     * The Enabled state. Initial state of the outer-state Processing.
     */
    class Enabled: public EvBState<Enabled,Processing>
    {
      
    public:
      
      typedef boost::mpl::list<
      boost::statechart::transition<MismatchDetected,MismatchDetectedBackPressuring,ReadoutUnitStateMachine,&ReadoutUnitStateMachine::mismatchEvent>,
      boost::statechart::transition<TimedOut,TimedOutBackPressuring,ReadoutUnitStateMachine,&ReadoutUnitStateMachine::timedOutEvent>
      > reactions;
    
      Enabled(my_context c) : my_state("Enabled", c)
      { safeEntryAction(); }
      virtual ~Enabled()
      { safeExitAction(); }
      
      virtual void entryAction();
      virtual void exitAction();
      
    };
    
    
    /**
     * The MismatchDetectedBackPressuring state of the outer-state Processing.
     */
    class MismatchDetectedBackPressuring: public EvBState<MismatchDetectedBackPressuring,Processing>
    {
      
    public:
      
      MismatchDetectedBackPressuring(my_context c) : my_state("MismatchDetectedBackPressuring", c)
      { safeEntryAction(); }
      virtual ~MismatchDetectedBackPressuring()
      { safeExitAction(); }
      
    };
    
    
    /**
     * The TimedOutBackPressuring state of the outer-state Processing.
     */
    class TimedOutBackPressuring: public EvBState<TimedOutBackPressuring,Processing>
    {
      
    public:
      
      TimedOutBackPressuring(my_context c) : my_state("TimedOutBackPressuring", c)
      { safeEntryAction(); }
      virtual ~TimedOutBackPressuring()
      { safeExitAction(); }
      
    };
    
    
  } } //namespace evb::readoutunit

#endif //_evb_readoutunit_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
