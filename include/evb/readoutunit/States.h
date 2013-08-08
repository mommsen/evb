#ifndef _evb_readoutunit_States_h_
#define _evb_readoutunit_States_h_

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/bind.hpp>
#include <boost/mpl/list.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>

#include "evb/EvBStateMachine.h"
#include "evb/Exception.h"
#include "evb/readoutunit/StateMachine.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"

#include <string>


namespace evb {

  namespace readoutunit {

    ///////////////////////////////////////////
    // Forward declarations of state classes //
    ///////////////////////////////////////////

    // Outer states:
    template<class> class Failed;
    template<class> class AllOk;
    // Inner states of AllOk
    template<class> class Halted;
    template<class> class Active;
    // Inner states of Active
    template<class> class Configuring;
    template<class> class Configured;
    template<class> class Processing;
    // Inner states of Configured
    template<class> class Clearing;
    template<class> class Ready;
    // Inner states of Processing
    template<class> class Enabled;
    // Inner states of Enabled
    template<class> class MismatchDetectedBackPressuring;

    template<class> class StateMachine;


    ///////////////////
    // State classes //
    ///////////////////

    /**
     * The outermost state
     */
    template<class Owner>
    class Outermost: public EvBState< Outermost<Owner>,StateMachine<Owner>,boost::mpl::list< AllOk<Owner> > >
    {

    public:

      typedef EvBState< Outermost<Owner>,StateMachine<Owner>,boost::mpl::list< AllOk<Owner> > > my_state;
      typedef boost::mpl::list<> reactions;

      Outermost(typename my_state::boost_state::my_context c) : my_state("Outermost", c)
      { this->safeEntryAction(); }
      virtual ~Outermost()
      { this->safeExitAction(); }

    };


    /**
     * Failed state
     */
    template<class Owner>
    class Failed: public EvBState< Failed<Owner>,Outermost<Owner> >
    {

    public:

      typedef EvBState< Failed<Owner>,Outermost<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< Fail,Failed<Owner> >
        > reactions;

      Failed(typename my_state::boost_state::my_context c) : my_state("Failed", c)
      { this->safeEntryAction(); }
      virtual ~Failed()
      { this->safeExitAction(); }

    };

    /**
     * The default state AllOk. Initial state of outer-state Outermost
     */
    template<class Owner>
    class AllOk: public EvBState< AllOk<Owner>,Outermost<Owner>,boost::mpl::list< Halted<Owner> > >
    {

    public:

      typedef EvBState< AllOk<Owner>,Outermost<Owner>,boost::mpl::list< Halted<Owner> > > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition<Fail,Failed<Owner>,
                                      EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >,
                                      &EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >::failEvent>
        > reactions;

      AllOk(typename my_state::boost_state::my_context c) : my_state("AllOk", c)
      { this->safeEntryAction(); }
      virtual ~AllOk()
      { this->safeExitAction(); }

    };


    /**
     * The Halted state. Initial state of outer-state AllOk.
     */
    template<class Owner>
    class Halted: public EvBState< Halted<Owner>,AllOk<Owner> >
    {

    public:

      typedef EvBState< Halted<Owner>,AllOk<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< Configure,Active<Owner> >
        > reactions;

      Halted(typename my_state::boost_state::my_context c) : my_state("Halted", c)
      { this->safeEntryAction(); }
      virtual ~Halted()
      { this->safeExitAction(); }

    };


    /**
     * The Active state of outer-state AllOk.
     */
    template<class Owner>
    class Active: public EvBState< Active<Owner>,AllOk<Owner>,boost::mpl::list< Configuring<Owner> > >
    {

    public:

      typedef EvBState< Active<Owner>,AllOk<Owner>,boost::mpl::list< Configuring<Owner> > > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< Halt,Halted<Owner> >
        > reactions;

      Active(typename my_state::boost_state::my_context c) : my_state("Active", c)
      { this->safeEntryAction(); }
      virtual ~Active()
      { this->safeExitAction(); }

    };


    /**
     * The Configuring state. Initial state of outer-state Active.
     */
    template<class Owner>
    class Configuring: public EvBState< Configuring<Owner>,Active<Owner> >
    {

    public:

      typedef EvBState< Configuring<Owner>,Active<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< ConfigureDone,Configured<Owner> >
        > reactions;

      Configuring(typename my_state::boost_state::my_context c) : my_state("Configuring", c)
      { this->safeEntryAction(); }
      virtual ~Configuring()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      void doConfigure() {};
      boost::scoped_ptr<boost::thread> configuringThread_;
      volatile bool doConfiguring_;

    };


    /**
     * The Configured state of the outer-state Active.
     */
    template<class Owner>
    class Configured: public EvBState< Configured<Owner>,Active<Owner>,boost::mpl::list< Clearing<Owner> > >
    {

    public:

      typedef EvBState< Configured<Owner>,Active<Owner>,boost::mpl::list< Clearing<Owner> > > my_state;

      Configured(typename my_state::boost_state::my_context c) : my_state("Configured", c)
      { this->safeEntryAction(); }
      virtual ~Configured()
      { this->safeExitAction(); }

    };


    /**
     * The Processing state of the outer-state Active.
     */
    template<class Owner>
    class Processing: public EvBState< Processing<Owner>,Active<Owner>,boost::mpl::list< Enabled<Owner> > >
    {

    public:

      typedef EvBState< Processing<Owner>,Active<Owner>,boost::mpl::list< Enabled<Owner> > > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< Stop,Configured<Owner> >
        > reactions;

      Processing(typename my_state::boost_state::my_context c) : my_state("Processing", c)
      { this->safeEntryAction(); }
      virtual ~Processing()
      { this->safeExitAction(); }

      virtual void entryAction();

    private:
      void doResetMonitoringCounters() {};

    };


    /**
     * The Clearing state. Initial state of outer-state Configured.
     */
    template<class Owner>
    class Clearing: public EvBState< Clearing<Owner>,Configured<Owner> >
    {

    public:

      typedef EvBState< Clearing<Owner>,Configured<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< ClearDone,Ready<Owner> >
        > reactions;

      Clearing(typename my_state::boost_state::my_context c) : my_state("Clearing", c)
      { this->safeEntryAction(); }
      virtual ~Clearing()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:

      void doClearing() {};
      boost::scoped_ptr<boost::thread> clearingThread_;
      volatile bool doClearing_;

    };


    /**
     * The Ready state of outer-state Configured.
     */
    template<class Owner>
    class Ready: public EvBState< Ready<Owner>,Configured<Owner> >
    {

    public:

      typedef EvBState< Ready<Owner>,Configured<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition< Enable,Processing<Owner> >
        > reactions;

      Ready(typename my_state::boost_state::my_context c) : my_state("Ready", c)
      { this->safeEntryAction(); }
      virtual ~Ready()
      { this->safeExitAction(); }

      virtual void entryAction()
      { this->outermost_context().notifyRCMS("Ready"); }

    };


    /**
     * The Enabled state. Initial state of the outer-state Processing.
     */
    template<class Owner>
    class Enabled: public EvBState< Enabled<Owner>,Processing<Owner> >
    {

    public:

      typedef EvBState< Enabled<Owner>,Processing<Owner> > my_state;
      typedef boost::mpl::list<
        boost::statechart::transition<MismatchDetected,MismatchDetectedBackPressuring<Owner>,
                                      StateMachine<Owner>,
                                      &StateMachine<Owner>::mismatchEvent>
        > reactions;

      Enabled(typename my_state::boost_state::my_context c) : my_state("Enabled", c)
      { this->safeEntryAction(); }
      virtual ~Enabled()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    private:
      void doStartProcessing(const uint32_t runNumber) {};
      void doStopProcessing() {};

    };


    /**
     * The MismatchDetectedBackPressuring state of the outer-state Processing.
     */
    template<class Owner>
    class MismatchDetectedBackPressuring: public EvBState< MismatchDetectedBackPressuring<Owner>,Processing<Owner> >
    {

    public:

      typedef EvBState< MismatchDetectedBackPressuring<Owner>,Processing<Owner> > my_state;

      MismatchDetectedBackPressuring(typename my_state::boost_state::my_context c) : my_state("MismatchDetectedBackPressuring", c)
      { this->safeEntryAction(); }
      virtual ~MismatchDetectedBackPressuring()
      { this->safeExitAction(); }

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////


template<class Owner>
void evb::readoutunit::Configuring<Owner>::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::Configuring<Owner>::activity, this) )
  );
  configuringThread_->detach();
}


template<class Owner>
void evb::readoutunit::Configuring<Owner>::activity()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.getOwner()->getInput()->configure();
    if (doConfiguring_) stateMachine.getOwner()->getBUproxy()->configure();
    if (doConfiguring_) doConfigure();

    if (doConfiguring_) stateMachine.processFSMEvent( ConfigureDone() );
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::FSM,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
}


template<class Owner>
void evb::readoutunit::Configuring<Owner>::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


template<class Owner>
void evb::readoutunit::Clearing<Owner>::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::Clearing<Owner>::activity, this) )
  );
  clearingThread_->detach();
}


template<class Owner>
void evb::readoutunit::Clearing<Owner>::activity()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();

  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.getOwner()->getBUproxy()->clear();
    if (doClearing_) stateMachine.getOwner()->getInput()->clear();
    if (doClearing_) doClearing();

    if (doClearing_) stateMachine.processFSMEvent( ClearDone() );
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::FSM,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
}


template<class Owner>
void evb::readoutunit::Clearing<Owner>::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


template<class Owner>
void evb::readoutunit::Processing<Owner>::entryAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  stateMachine.getOwner()->getBUproxy()->resetMonitoringCounters();
  stateMachine.getOwner()->getInput()->resetMonitoringCounters();
  doResetMonitoringCounters();
}


template<class Owner>
void evb::readoutunit::Enabled<Owner>::entryAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();

  const uint32_t runNumber = stateMachine.getRunNumber();

  stateMachine.getOwner()->getInput()->startProcessing(runNumber);
  stateMachine.getOwner()->getBUproxy()->startProcessing();
  doStartProcessing(runNumber);
}


template<class Owner>
void evb::readoutunit::Enabled<Owner>::exitAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  stateMachine.getOwner()->getBUproxy()->stopProcessing();
  stateMachine.getOwner()->getInput()->stopProcessing();
  doStopProcessing();
}


#endif //_evb_readoutunit_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
