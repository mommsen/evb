#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "evb/readoutunit/States.h"
#include "evb/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


void evb::readoutunit::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::Configuring::activity, this) )
  );
  configuringThread_->detach();
}


void evb::readoutunit::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.getReadoutUnit()->getInput()->configure();
    if (doConfiguring_) stateMachine.getReadoutUnit()->getBUproxy()->configure();
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


void evb::readoutunit::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::readoutunit::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &evb::readoutunit::Clearing::activity, this) )
  );
  clearingThread_->detach();
}


void evb::readoutunit::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.getReadoutUnit()->getBUproxy()->clear();
    if (doClearing_) stateMachine.getReadoutUnit()->getInput()->clear();
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


void evb::readoutunit::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void evb::readoutunit::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.getReadoutUnit()->getBUproxy()->resetMonitoringCounters();
  stateMachine.getReadoutUnit()->getInput()->resetMonitoringCounters();
}


void evb::readoutunit::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  const uint32_t runNumber = stateMachine.getRunNumber();
  
  stateMachine.getReadoutUnit()->getInput()->startProcessing(runNumber);
  stateMachine.getReadoutUnit()->getBUproxy()->startProcessing();
}


void evb::readoutunit::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.getReadoutUnit()->getBUproxy()->stopProcessing();
  stateMachine.getReadoutUnit()->getInput()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
