#include "evb/Exception.h"
#include "evb/test/DummyFEROL.h"
#include "evb/test/dummyFEROL/StateMachine.h"
#include "evb/test/dummyFEROL/States.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


evb::test::dummyFEROL::StateMachine::StateMachine
(
  DummyFEROL* dummyFEROL
):
EvBStateMachine(dummyFEROL),
dummyFEROL_(dummyFEROL)
{}


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


void evb::test::dummyFEROL::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::test::dummyFEROL::Running::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.dummyFEROL()->startProcessing();
}


void evb::test::dummyFEROL::Running::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.dummyFEROL()->stopProcessing();
}


void evb::test::dummyFEROL::Draining::entryAction()
{
  doDraining_ = true;
  drainingThread_.reset(
    new boost::thread( boost::bind( &evb::test::dummyFEROL::Draining::activity, this) )
  );
  drainingThread_->detach();
}


void evb::test::dummyFEROL::Draining::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to drain the components";
  try
  {
    if (doDraining_) stateMachine.dummyFEROL()->drain();
    if (doDraining_) stateMachine.processFSMEvent( DrainingDone() );
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


void evb::test::dummyFEROL::Draining::exitAction()
{
  doDraining_ = false;
  drainingThread_->join();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
