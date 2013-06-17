#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/EventTable.h"
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
  EventTablePtr eventTable,
  boost::shared_ptr<ResourceManager> resourceManager
):
EvBStateMachine(bu),
bu_(bu),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
eventTable_(eventTable),
resourceManager_(resourceManager)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void evb::bu::StateMachine::do_appendConfigurationItems(InfoSpaceItems& params)
{
  runNumber_ = 0;
  maxEvtsUnderConstruction_ = 64;
  
  params.add("runNumber", &runNumber_);
  params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction_);
}


void evb::bu::StateMachine::do_appendMonitoringItems(InfoSpaceItems& items)
{
  items.add("runNumber", &runNumber_);
}


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
  
  std::string msg = "Failed to get descriptors of peer applications";
  try
  {
    if (doConfiguring_) stateMachine.ruProxy()->getApplicationDescriptors();
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


  msg = "Failed to configure the components";
  try
  {  
    const uint32_t maxEvtsUnderConstruction = stateMachine.maxEvtsUnderConstruction();
    
    if (doConfiguring_) stateMachine.bu()->configure();
    if (doConfiguring_) stateMachine.ruProxy()->configure();
    if (doConfiguring_) stateMachine.diskWriter()->configure(maxEvtsUnderConstruction);
    if (doConfiguring_) stateMachine.eventTable()->configure();
    if (doConfiguring_) stateMachine.resourceManager()->configure(maxEvtsUnderConstruction);
    
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


void evb::bu::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void evb::bu::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &evb::bu::Clearing::activity, this) )
  );
  clearingThread_->detach();
}


void evb::bu::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.bu()->clear();
    if (doClearing_) stateMachine.ruProxy()->clear();
    if (doClearing_) stateMachine.diskWriter()->clear();
    if (doClearing_) stateMachine.eventTable()->clear();
    if (doClearing_) stateMachine.resourceManager()->clear();
    
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


void evb::bu::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void evb::bu::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();

  stateMachine.ruProxy()->resetMonitoringCounters();
  stateMachine.diskWriter()->resetMonitoringCounters();
  stateMachine.resourceManager()->resetMonitoringCounters();
}


void evb::bu::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  const uint32_t runNumber = stateMachine.runNumber();

  stateMachine.diskWriter()->startProcessing(runNumber);
  stateMachine.eventTable()->startProcessing(runNumber);
  stateMachine.ruProxy()->startProcessing();
}


void evb::bu::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.ruProxy()->stopProcessing();
  stateMachine.eventTable()->stopProcessing();
  stateMachine.diskWriter()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
