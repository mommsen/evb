#ifndef _evb_readoutunit_States_h_
#define _evb_readoutunit_States_h_

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/mpl/list.hpp>
#include <boost/thread/thread.hpp>

#include "evb/EvBStateMachine.h"
#include "evb/Exception.h"
#include "evb/readoutunit/StateMachine.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"

#include <memory>
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
    template<class> class Ready;
    template<class> class Running;
    // Inner states of Running
    template<class> class Enabled;
    template<class> class Draining;
    template<class> class SyncLoss;
    template<class> class IncompleteEvents;
    // Inner statees of IncompleteEvents
    template<class> class MissingData;

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

      using my_state = EvBState< Outermost<Owner>,StateMachine<Owner>,boost::mpl::list< AllOk<Owner> > >;
      using reactions = boost::mpl::list<
        boost::statechart::in_state_reaction< DrainingDone >
        >;

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

      using my_state = EvBState< Failed<Owner>,Outermost<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Halt,Halted<Owner> >,
        boost::statechart::in_state_reaction< Fail >
        >;

      Failed(typename my_state::boost_state::my_context c) : my_state("Failed", c)
      { this->safeEntryAction(); }
      virtual ~Failed()
      { this->safeExitAction(); }

      virtual void exitAction()
      { this->outermost_context().clearError(); }

    };

    /**
     * The default state AllOk. Initial state of outer-state Outermost
     */
    template<class Owner>
    class AllOk: public EvBState< AllOk<Owner>,Outermost<Owner>,boost::mpl::list< Halted<Owner> > >
    {

    public:

      using my_state = EvBState< AllOk<Owner>,Outermost<Owner>,boost::mpl::list< Halted<Owner> > >;
      using reactions = boost::mpl::list<
        boost::statechart::transition<Fail,Failed<Owner>,
                                      EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >,
                                      &EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >::failEvent>
        >;

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

      using my_state = EvBState< Halted<Owner>,AllOk<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Configure,Active<Owner> >,
        boost::statechart::in_state_reaction< Halt >
        >;

      Halted(typename my_state::boost_state::my_context c) : my_state("Halted", c)
      { this->safeEntryAction(); }
      virtual ~Halted()
      { this->safeExitAction(); }

      virtual void entryAction();

    };


    /**
     * The Active state of outer-state AllOk.
     */
    template<class Owner>
    class Active: public EvBState< Active<Owner>,AllOk<Owner>,boost::mpl::list< Configuring<Owner> > >
    {

    public:

      using my_state = EvBState< Active<Owner>,AllOk<Owner>,boost::mpl::list< Configuring<Owner> > >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Halt,Halted<Owner> >
        >;

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

      using my_state = EvBState< Configuring<Owner>,Active<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< ConfigureDone,Ready<Owner> >
        >;

      Configuring(typename my_state::boost_state::my_context c) : my_state("Configuring", c)
      { this->safeEntryAction(); }
      virtual ~Configuring()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:
      void doConfigure(const Owner*) {};
      std::unique_ptr<boost::thread> configuringThread_;
      volatile bool doConfiguring_;

    };


    /**
     * The Ready state of outer-state Active.
     */
    template<class Owner>
    class Ready: public EvBState< Ready<Owner>,Active<Owner> >
    {

    public:

      using my_state = EvBState< Ready<Owner>,Active<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Enable,Enabled<Owner> >,
        boost::statechart::in_state_reaction< Clear >
        >;

      Ready(typename my_state::boost_state::my_context c) : my_state("Ready", c)
      { this->safeEntryAction(); }
      virtual ~Ready()
      { this->safeExitAction(); }

      virtual void entryAction()
      { this->outermost_context().notifyRCMS("Ready"); }

    };


    /**
     * The Running state of the outer-state Active.
     */
    template<class Owner>
    class Running: public EvBState< Running<Owner>,Active<Owner>,boost::mpl::list< Enabled<Owner> > >
    {

    public:

      using my_state = EvBState< Running<Owner>,Active<Owner>,boost::mpl::list< Enabled<Owner> > >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Clear,Configuring<Owner> >
        >;

      Running(typename my_state::boost_state::my_context c) : my_state("Running", c)
      { this->safeEntryAction(); }
      virtual ~Running()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    private:

      void doStartProcessing(const Owner*, const uint32_t runNumber) {};
      void doStopProcessing(const Owner*) {};

    };


    /**
     * The Enabled state. Initial state of the outer-state Running.
     */
    template<class Owner>
    class Enabled: public EvBState< Enabled<Owner>,Running<Owner> >
    {

    public:

      using my_state = EvBState< Enabled<Owner>,Running<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Stop,Draining<Owner> >,
        boost::statechart::transition< MismatchDetected,SyncLoss<Owner>,
                                       StateMachine<Owner>,
                                       &StateMachine<Owner>::mismatchDetected>,
        boost::statechart::transition< EventOutOfSequence,SyncLoss<Owner>,
                                       StateMachine<Owner>,
                                       &StateMachine<Owner>::eventOutOfSequence>,
        boost::statechart::transition< DataLoss,MissingData<Owner>,
                                       StateMachine<Owner>,
                                       &StateMachine<Owner>::dataLoss>
        >;

      Enabled(typename my_state::boost_state::my_context c) : my_state("Enabled", c)
      { this->safeEntryAction(); }
      virtual ~Enabled()
      { this->safeExitAction(); }

      virtual void entryAction();

    };


    /**
     * The Draining state of the outer-state Running.
     */
    template<class Owner>
    class Draining: public EvBState< Draining<Owner>,Running<Owner> >
    {

    public:

      using my_state = EvBState< Draining<Owner>,Running<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< DrainingDone,Ready<Owner> >
        >;

      Draining(typename my_state::boost_state::my_context c) : my_state("Draining", c)
      { this->safeEntryAction(); }
      virtual ~Draining()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();
      void activity();

    private:

      void doDraining(const Owner*) {};
      std::unique_ptr<boost::thread> drainingThread_;
      volatile bool doDraining_;

    };


    /**
     * The SyncLoss state of the outer-state Running.
     */
    template<class Owner>
    class SyncLoss: public EvBState< SyncLoss<Owner>,Running<Owner> >
    {

    public:

      using my_state = EvBState< SyncLoss<Owner>,Running<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Stop,Draining<Owner> >,
        boost::statechart::in_state_reaction< MismatchDetected >,
        boost::statechart::in_state_reaction< EventOutOfSequence >
        >;

      SyncLoss(typename my_state::boost_state::my_context c) : my_state("SyncLoss", c)
      { this->safeEntryAction(); }
      virtual ~SyncLoss()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    };


    /**
     * The IncompleteEvents state of the outer-state Running.
     */
    template<class Owner>
    class IncompleteEvents: public EvBState< IncompleteEvents<Owner>,Running<Owner>,boost::mpl::list< MissingData<Owner> > >
    {

    public:

      using my_state = EvBState< IncompleteEvents<Owner>,Running<Owner>,boost::mpl::list< MissingData<Owner> > >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< Stop,Draining<Owner> >,
        boost::statechart::transition< Recovered,Enabled<Owner> >
        >;

      IncompleteEvents(typename my_state::boost_state::my_context c) : my_state("IncompleteEvents", c)
      { this->safeEntryAction(); }
      virtual ~IncompleteEvents()
      { this->safeExitAction(); }

      virtual void entryAction();
      virtual void exitAction();

    private:

      void timeoutActivity();
      std::unique_ptr<boost::thread> timeoutThread_;

    };


    /**
     * The MissingData state of the outer-state IncompleteEvents.
     */
    template<class Owner>
    class MissingData: public EvBState< MissingData<Owner>,IncompleteEvents<Owner> >
    {

    public:

      using my_state = EvBState< MissingData<Owner>,IncompleteEvents<Owner> >;
      using reactions = boost::mpl::list<
        boost::statechart::transition< DataLoss,MissingData<Owner>,
                                       StateMachine<Owner>,
                                       &StateMachine<Owner>::dataLoss>,
        boost::statechart::custom_reaction< Recovered >
        >;

      MissingData(typename my_state::boost_state::my_context c) : my_state("MissingData", c)
      { this->safeEntryAction(); }
      virtual ~MissingData()
      { this->safeExitAction(); }

      virtual boost::statechart::result react(const Recovered&);

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Owner>
void evb::readoutunit::Halted<Owner>::entryAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const Owner* owner = stateMachine.getOwner();
  owner->getFerolConnectionManager()->dropConnections();
  owner->getFerolConnectionManager()->acceptConnections();
}


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
  const Owner* owner = stateMachine.getOwner();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) owner->getInput()->configure();
    if (doConfiguring_) owner->getBUproxy()->configure();
    if (doConfiguring_) doConfigure(owner);

    if (doConfiguring_) stateMachine.processFSMEvent( ConfigureDone() );
  }
  catch(xcept::Exception& e)
  {
    XCEPT_DECLARE_NESTED(exception::FSM,
                         sentinelException, msg, e);
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
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
}


template<class Owner>
void evb::readoutunit::Running<Owner>::entryAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const Owner* owner = stateMachine.getOwner();
  const uint32_t runNumber = stateMachine.getRunNumber();
  stateMachine.resetMissingFeds();

  owner->getFerolConnectionManager()->startProcessing();
  owner->getInput()->startProcessing(runNumber);
  owner->getBUproxy()->startProcessing();
  doStartProcessing(owner,runNumber);
}


template<class Owner>
void evb::readoutunit::Running<Owner>::exitAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const Owner* owner = stateMachine.getOwner();

  owner->getInput()->stopProcessing();
  owner->getBUproxy()->stopProcessing();
  doStopProcessing(owner);
}


template<class Owner>
void evb::readoutunit::Enabled<Owner>::entryAction()
{
  this->outermost_context().notifyRCMS("Enabled");
}


template<class Owner>
void evb::readoutunit::Draining<Owner>::entryAction()
{
  doDraining_ = true;
  drainingThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::Draining<Owner>::activity, this) )
  );
  drainingThread_->detach();
}


template<class Owner>
void evb::readoutunit::Draining<Owner>::activity()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const Owner* owner = stateMachine.getOwner();

  std::string msg = "Failed to drain the components";
  try
  {
    if (doDraining_) owner->getInput()->drain();
    if (doDraining_) owner->getFerolConnectionManager()->drain();
    if (doDraining_) owner->getBUproxy()->drain();
    if (doDraining_) doDraining(owner);

    if (doDraining_) stateMachine.processFSMEvent( DrainingDone() );
  }
  catch(xcept::Exception& e)
  {
    XCEPT_DECLARE_NESTED(exception::FSM,
                         sentinelException, msg, e);
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
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
void evb::readoutunit::Draining<Owner>::exitAction()
{
  doDraining_ = false;
}


template<class Owner>
void evb::readoutunit::SyncLoss<Owner>::entryAction()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const Owner* owner = stateMachine.getOwner();

  owner->getInput()->blockInput();
}


template<class Owner>
void evb::readoutunit::SyncLoss<Owner>::exitAction()
{
  this->outermost_context().clearError();
}


template<class Owner>
void evb::readoutunit::IncompleteEvents<Owner>::entryAction()
{
  timeoutThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::IncompleteEvents<Owner>::timeoutActivity, this) )
  );
}


template<class Owner>
void evb::readoutunit::IncompleteEvents<Owner>::exitAction()
{
  timeoutThread_->interrupt();
  timeoutThread_->detach();
  this->outermost_context().clearError();
}


template<class Owner>
void evb::readoutunit::IncompleteEvents<Owner>::timeoutActivity()
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  const boost::posix_time::time_duration maxTimeWithIncompleteEvents =
    boost::posix_time::seconds(stateMachine.getOwner()->getConfiguration()->maxTimeWithIncompleteEvents);
  const Owner* owner = stateMachine.getOwner();

  if ( maxTimeWithIncompleteEvents.total_seconds() == 0 ) return;

  try
  {
    do
      boost::this_thread::sleep(maxTimeWithIncompleteEvents);
    while ( owner->getInput()->getEventRate() == 0 );
  }
  catch(boost::thread_interrupted)
  {
    // State was exited before timeout kicked in
    return;
  }

  std::ostringstream msg;
  msg << "Built incomplete events for more than " << maxTimeWithIncompleteEvents.total_seconds() << " seconds";
  XCEPT_DECLARE(exception::FSM,
                sentinelException, msg.str() );
  stateMachine.processFSMEvent( Fail(sentinelException) );
}


template<class Owner>
boost::statechart::result evb::readoutunit::MissingData<Owner>::react(const Recovered& event)
{
  typename my_state::outermost_context_type& stateMachine = this->outermost_context();
  std::ostringstream msg;
  msg << "FED " << event.getFedId() << " (" << stateMachine.getOwner()->getSubSystem() << ") resynced";

  if ( stateMachine.removeMissingFed( event.getFedId() ) )
  {
    msg << ", going back to Enabled";
    LOG4CPLUS_INFO(stateMachine.getLogger(),msg.str());
    return this->template transit< Enabled<Owner> >();
  }
  else
  {
    LOG4CPLUS_INFO(stateMachine.getLogger(),msg.str());
    return this->template discard_event();
  }
}


#endif //_evb_readoutunit_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
