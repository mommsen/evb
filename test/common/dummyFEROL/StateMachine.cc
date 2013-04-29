#include "evb/Exception.h"
#include "evb/test/DummyFEROL.h"
#include "evb/test/dummyFEROL/States.h"
#include "evb/test/dummyFEROL/StateMachine.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


evb::test::dummyFEROL::StateMachine::StateMachine
(
  DummyFEROL* dummyFEROL
):
EvBStateMachine(dummyFEROL),
dummyFEROL_(dummyFEROL)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void evb::test::dummyFEROL::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &evb::test::dummyFEROL::Configuring::activity, this) )
  );
  configuringThread_->detach();
}


void evb::test::dummyFEROL::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.dummyFEROL()->configure();
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


void evb::test::dummyFEROL::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::test::dummyFEROL::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &evb::test::dummyFEROL::Clearing::activity, this) )
  );
  clearingThread_->detach();
}


void evb::test::dummyFEROL::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.dummyFEROL()->clear();
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


void evb::test::dummyFEROL::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void evb::test::dummyFEROL::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.dummyFEROL()->resetMonitoringCounters();
}


void evb::test::dummyFEROL::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.dummyFEROL()->startProcessing();
}


void evb::test::dummyFEROL::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.dummyFEROL()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
