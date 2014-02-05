#ifndef _evb_StateMachine_h_
#define _evb_StateMachine_h_

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
#include "xcept/tools.h"
#include "xdaq/Application.h"
#include "xdaq2rc/RcmsStateNotifier.h"
#include "xdata/String.h"

#include <string>


namespace evb {

  ///////////////////
  // Public events //
  ///////////////////
  class Configure : public boost::statechart::event<Configure> {};
  class Halt : public boost::statechart::event<Halt> {};
  class Enable : public boost::statechart::event<Enable> {};
  class Stop : public boost::statechart::event<Stop> {};

  ////////////////////////////////
  // Internal transition events //
  ////////////////////////////////

  class ConfigureDone: public boost::statechart::event<ConfigureDone> {};
  class DrainingDone: public boost::statechart::event<DrainingDone> {};

  class Fail : public boost::statechart::event<Fail>
  {
  public:
    Fail(xcept::Exception& exception) : exception_(exception) {};
    std::string getReason() const { return exception_.message(); }
    std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
    xcept::Exception& getException() const { return exception_; }

  private:
    mutable xcept::Exception exception_;
  };

  struct StateName {
    virtual ~StateName() {};
    virtual std::string stateName() const = 0;
  };

  /////////////////////////////////
  // The evb state machine //
  /////////////////////////////////

  template<class MostDerived, class InitialState>
  class EvBStateMachine :
    public boost::statechart::state_machine<MostDerived,InitialState>
  {

  public:

    EvBStateMachine(xdaq::Application*);

    void processSoapEvent(const std::string& event, std::string& newStateName);
    std::string processFSMEvent(const boost::statechart::event_base&);
    void processEvent(const boost::statechart::event_base&);
    void fail(xcept::Exception&);

    void appendConfigurationItems(InfoSpaceItems&);
    void appendMonitoringItems(InfoSpaceItems&);
    void updateMonitoringItems();

    void notifyRCMS(const std::string& stateName);
    void failEvent(const Fail&);
    void unconsumed_event(const boost::statechart::event_base&);
    log4cplus::Logger& getLogger() const { return app_->getApplicationLogger(); }

    std::string getStateName() const { return stateName_; }
    uint32_t getRunNumber() const { return runNumber_.value_; }

    typedef std::list<std::string> SoapFsmEvents;
    SoapFsmEvents getSoapFsmEvents() const { return soapFsmEvents_; }

  protected:

    virtual void do_processSoapEvent(const std::string& event, std::string& newStateName);

    xdaq::Application* app_;
    xdaq2rc::RcmsStateNotifier rcmsStateNotifier_;

    SoapFsmEvents soapFsmEvents_;

  private:

    boost::shared_mutex eventMutex_;
    boost::shared_mutex stateNameMutex_;

    xdata::UnsignedInteger32 runNumber_;
    std::string stateName_;

    std::string reasonForFailed_;
    xdata::UnsignedInteger32 monitoringRunNumber_;
    xdata::String monitoringStateName_;
  };


  ////////////////////////////
  // Wrapper state template //
  ////////////////////////////

  template< class MostDerived,
            class Context,
            class InnerInitial = boost::mpl::list<>,
            boost::statechart::history_mode historyMode = boost::statechart::has_no_history >
  class EvBState : public StateName,
                   public boost::statechart::state<MostDerived, Context, InnerInitial, historyMode>
  {
  public:
    std::string stateName() const
    { return stateName_; }

  protected:
    typedef boost::statechart::state<MostDerived, Context, InnerInitial, historyMode> boost_state;
    typedef EvBState my_state;

    EvBState(const std::string stateName, typename boost_state::my_context& c) :
    boost_state(c), stateName_(stateName) {};
    virtual ~EvBState() {};

    virtual void entryAction() {};
    virtual void exitAction() {};

    const std::string stateName_;

    void safeEntryAction()
    {
      std::string msg = "Failed to enter " + stateName_ + " state";
      try
      {
        entryAction();
      }
      catch(xcept::Exception& e)
      {
        XCEPT_DECLARE_NESTED(exception::FSM,
          sentinelException, msg, e);
        this->post_event( Fail(sentinelException) );
      }
      catch(std::exception& e)
      {
        msg += ": ";
        msg += e.what();
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
      catch(...)
      {
        msg += ": unknown exception";
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
    };

    void safeExitAction()
    {
      std::string msg = "Failed to leave " + stateName_ + " state";
      try
      {
        exitAction();
      }
      catch(xcept::Exception& e)
      {
        XCEPT_DECLARE_NESTED(exception::FSM,
          sentinelException, msg, e);
        this->post_event( Fail(sentinelException) );
      }
      catch(std::exception& e)
      {
        msg += ": ";
        msg += e.what();
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
      catch(...)
      {
        msg += ": unknown exception";
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
    };

  };

} //namespace evb

////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class MostDerived,class InitialState>
evb::EvBStateMachine<MostDerived,InitialState>::EvBStateMachine
(
  xdaq::Application* app
):
app_(app),
rcmsStateNotifier_
(
  getLogger(),
  app->getApplicationDescriptor(),
  app->getApplicationContext()
),
runNumber_(0),
stateName_("Halted"),
reasonForFailed_("")
{
  xdata::InfoSpace *is = app->getApplicationInfoSpace();
  is->fireItemAvailable("rcmsStateListener",
    rcmsStateNotifier_.getRcmsStateListenerParameter() );
  is->fireItemAvailable("foundRcmsStateListener",
    rcmsStateNotifier_.getFoundRcmsStateListenerParameter() );
  rcmsStateNotifier_.findRcmsStateListener();
  rcmsStateNotifier_.subscribeToChangesInRcmsStateListener(is);

  soapFsmEvents_.push_back("Configure");
  soapFsmEvents_.push_back("Halt");
  soapFsmEvents_.push_back("Enable");
  soapFsmEvents_.push_back("Stop");
  soapFsmEvents_.push_back("Fail");
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::processSoapEvent
(
  const std::string& soapEvent,
  std::string& newStateName
)
{
  do_processSoapEvent(soapEvent, newStateName);
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::do_processSoapEvent
(
  const std::string& soapEvent,
  std::string& newStateName
)
{
  if ( soapEvent == "Configure" )
    newStateName = processFSMEvent( Configure() );
  else if ( soapEvent == "Enable" )
    newStateName = processFSMEvent( Enable() );
  else if ( soapEvent == "Halt" )
    newStateName = processFSMEvent( Halt() );
  else if ( soapEvent == "Stop" )
    newStateName = processFSMEvent( Stop() );
  else if ( soapEvent == "Fail" )
  {
    XCEPT_DECLARE(exception::FSM,
      sentinelException, "Externally requested by SOAP command" );
    newStateName = processFSMEvent( Fail(sentinelException) );
  }
  else
  {
    XCEPT_DECLARE(exception::FSM, sentinelException,
      "Received an unknown state machine event '" + soapEvent + "'");
    newStateName = processFSMEvent( Fail(sentinelException) );
  }
}


template <class MostDerived,class InitialState>
std::string evb::EvBStateMachine<MostDerived,InitialState>::processFSMEvent
(
  const boost::statechart::event_base& event
)
{
  boost::unique_lock<boost::shared_mutex> eventUniqueLock(eventMutex_);
  this->process_event(event);

  boost::unique_lock<boost::shared_mutex> stateNameUniqueLock(stateNameMutex_);
  stateName_ = this->template state_cast<const StateName&>().stateName();
  return stateName_;
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::processEvent
(
  const boost::statechart::event_base& event
)
{
  boost::shared_lock<boost::shared_mutex> eventSharedLock(eventMutex_);
  this->process_event(event);
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::fail
(
  xcept::Exception& sentinelException
)
{
  this->post_event( Fail(sentinelException) );
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::appendConfigurationItems
(
  InfoSpaceItems& params
)
{
  runNumber_ = 0;

  params.add("runNumber", &runNumber_);
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::appendMonitoringItems
(
  InfoSpaceItems& items
)
{
  monitoringRunNumber_ = 0;
  monitoringStateName_ = "Halted";

  items.add("runNumber", &monitoringRunNumber_);
  items.add("stateName", &monitoringStateName_);
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::updateMonitoringItems()
{
  boost::shared_lock<boost::shared_mutex> stateNameSharedLock(stateNameMutex_);
  monitoringStateName_ = stateName_;
  monitoringRunNumber_ = runNumber_;
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::notifyRCMS
(
  const std::string& stateName
)
{
  rcmsStateNotifier_.stateChanged(stateName, "New state is " + stateName);
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::failEvent(const Fail& evt)
{
  reasonForFailed_ = evt.getTraceback();

  LOG4CPLUS_FATAL(getLogger(),
    "Failed: " << evt.getReason() << ". " << reasonForFailed_);

  rcmsStateNotifier_.stateChanged("Failed", evt.getReason());

  app_->notifyQualified("fatal", evt.getException());

  boost::unique_lock<boost::shared_mutex> stateNameUniqueLock(stateNameMutex_);
  stateName_ = "Failed";
}


template <class MostDerived,class InitialState>
void evb::EvBStateMachine<MostDerived,InitialState>::unconsumed_event
(
  const boost::statechart::event_base& evt
)
{
  boost::shared_lock<boost::shared_mutex> stateNameSharedLock(stateNameMutex_);

  LOG4CPLUS_ERROR(getLogger(),
    "The " << typeid(evt).name()
    << " event is not supported from the "
    << stateName_ << " state!");
}


#endif //_evb_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
