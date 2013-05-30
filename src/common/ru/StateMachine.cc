#include "evb/PerformanceMonitor.h"
#include "evb/RU.h"
#include "evb/ru/BUproxy.h"
#include "evb/ru/Input.h"
#include "evb/ru/StateMachine.h"
#include "evb/ru/States.h"
#include "evb/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


evb::ru::StateMachine::StateMachine
(
  RU* ru,
  boost::shared_ptr<Input> ruInput,
  boost::shared_ptr<BUproxy> buProxy
):
EvBStateMachine(ru),
ru_(ru),
ruInput_(ruInput),
buProxy_(buProxy)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void evb::ru::StateMachine::do_appendConfigurationItems(InfoSpaceItems& params)
{
  runNumber_ = 0;
  
  params.add("runNumber", &runNumber_);
}


void evb::ru::StateMachine::do_appendMonitoringItems(InfoSpaceItems& items)
{
  items.add("runNumber", &runNumber_);
}


void evb::ru::StateMachine::mismatchEvent(const MismatchDetected& evt)
{
  LOG4CPLUS_ERROR(app_->getApplicationLogger(), evt.getTraceback());
  
  rcmsStateNotifier_.stateChanged("MismatchDetectedBackPressuring", evt.getReason());

  app_->notifyQualified("error", evt.getException());
}


void evb::ru::StateMachine::timedOutEvent(const TimedOut& evt)
{
  LOG4CPLUS_ERROR(app_->getApplicationLogger(), evt.getTraceback());
  
  rcmsStateNotifier_.stateChanged("TimedOutBackPressuring", evt.getReason());

  app_->notifyQualified("error", evt.getException());
}


void evb::ru::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &evb::ru::Configuring::activity, this) )
  );
  configuringThread_->detach();
}


void evb::ru::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.buProxy()->configure();
    if (doConfiguring_) stateMachine.ru()->configure();
    if (doConfiguring_) stateMachine.ruInput()->configure();
    if (doConfiguring_) stateMachine.processFSMEvent( ConfigureDone() );
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::Configuration,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( Fail(sentinelException) );
  }
}


void evb::ru::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::ru::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &evb::ru::Clearing::activity, this) )
  );
  clearingThread_->detach();
}


void evb::ru::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.buProxy()->clear();
    if (doClearing_) stateMachine.ru()->clear();
    if (doClearing_) stateMachine.ruInput()->clear();
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


void evb::ru::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void evb::ru::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.buProxy()->resetMonitoringCounters();
  stateMachine.ru()->resetMonitoringCounters();
  stateMachine.ruInput()->resetMonitoringCounters();
}


void evb::ru::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  const uint32_t runNumber = stateMachine.runNumber();
  
  stateMachine.ruInput()->startProcessing(runNumber);
}


void evb::ru::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.ruInput()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
