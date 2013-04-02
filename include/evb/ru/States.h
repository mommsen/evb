#ifndef _evb_ru_States_h_
#define _evb_ru_States_h_

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


namespace evb { namespace ru {
  
  
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
    boost::statechart::custom_reaction<EvmRuDataReady>,
    boost::statechart::custom_reaction<RuSend>
    > reactions;

    Outermost(my_context c) : my_state("Outermost", c)
    { safeEntryAction(); }
    virtual ~Outermost()
    { safeExitAction(); }

    inline boost::statechart::result react(const EvmRuDataReady& evt)
    {
      evt.getDataReadyMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_EVMRU_DATA_READY message.");
      return discard_event();
    }

    inline boost::statechart::result react(const RuSend& evt)
    {
      evt.getSendMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_RU_SEND message.");
      return discard_event();
    }

  };


  /**
   * Failed state
   */
  class Failed: public EvBState<Failed,Outermost>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<Fail,Failed>,
    boost::statechart::custom_reaction<EvmRuDataReady>
    > reactions;

    Failed(my_context c) : my_state("Failed", c)
    { safeEntryAction(); }
    virtual ~Failed()
    { safeExitAction(); }
    
    inline boost::statechart::result react(const EvmRuDataReady& evt)
    {
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Got an I2O_EVMRU_DATA_READY message in Failed state.");
      return discard_event();
    }

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
    boost::statechart::transition<Stop,Configured>,
    boost::statechart::custom_reaction<EvmRuDataReady>,
    boost::statechart::custom_reaction<RuSend>
    > reactions;
    
    Processing(my_context c) : my_state("Processing", c)
    { safeEntryAction(); }
    virtual ~Processing()
    { safeExitAction(); }

    virtual void entryAction();

    inline boost::statechart::result react(const EvmRuDataReady& evt)
    {
      outermost_context().evmRuDataReady(evt.getDataReadyMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const RuSend& evt)
    {
      outermost_context().ruSend(evt.getSendMsg());
      return discard_event();
    }

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
    boost::statechart::transition<MismatchDetected,MismatchDetectedBackPressuring,StateMachine,&StateMachine::mismatchEvent>,
    boost::statechart::transition<TimedOut,TimedOutBackPressuring,StateMachine,&StateMachine::timedOutEvent>
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

  
} } //namespace evb::ru

#endif //_evb_ru_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
