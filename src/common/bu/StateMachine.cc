#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/EventBuilder.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/bu/States.h"
#include "evb/Constants.h"
#include "evb/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


evb::bu::StateMachine::StateMachine
(
  BU* bu,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<DiskWriter> diskWriter,
  EventBuilderPtr eventBuilder,
  boost::shared_ptr<ResourceManager> resourceManager
):
EvBStateMachine(bu),
bu_(bu),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
eventBuilder_(eventBuilder),
resourceManager_(resourceManager)
{}


void evb::bu::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &evb::bu::Configuring::activity, this) )
  );
  configuringThread_->detach();
}


void evb::bu::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    outermost_context_type& stateMachine = outermost_context();

    if (doConfiguring_) stateMachine.resourceManager()->configure();
    if (doConfiguring_) stateMachine.ruProxy()->configure();
    if (doConfiguring_) stateMachine.eventBuilder()->configure();
    if (doConfiguring_) stateMachine.diskWriter()->configure();

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


void evb::bu::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::bu::Running::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  const uint32_t runNumber = stateMachine.getRunNumber();

  stateMachine.resourceManager()->resetMonitoringCounters();

  stateMachine.bu()->processI2O(true);
  stateMachine.diskWriter()->startProcessing(runNumber);
  stateMachine.resourceManager()->startProcessing();
  stateMachine.eventBuilder()->startProcessing(runNumber);
  stateMachine.ruProxy()->startProcessing();
}


void evb::bu::Running::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();

  stateMachine.bu()->processI2O(false);
  stateMachine.ruProxy()->stopProcessing();
  stateMachine.eventBuilder()->stopProcessing();
  stateMachine.resourceManager()->stopProcessing();
  stateMachine.diskWriter()->stopProcessing();
}


void evb::bu::Draining::entryAction()
{
  doDraining_ = true;
  drainingThread_.reset(
    new boost::thread( boost::bind( &evb::bu::Draining::activity, this) )
  );
  drainingThread_->detach();
}


void evb::bu::Draining::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to drain the components";
  try
  {
    if (doDraining_) stateMachine.ruProxy()->drain();
    if (doDraining_) stateMachine.eventBuilder()->drain();
    if (doDraining_) stateMachine.resourceManager()->drain();
    if (doDraining_) stateMachine.diskWriter()->drain();

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


void evb::bu::Draining::exitAction()
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
