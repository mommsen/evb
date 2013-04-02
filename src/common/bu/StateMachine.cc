#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/FUproxy.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/StateMachine.h"
#include "evb/bu/States.h"
#include "evb/Constants.h"
#include "evb/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


evb::bu::StateMachine::StateMachine
(
  boost::shared_ptr<BU> bu,
  boost::shared_ptr<FUproxy> fuProxy,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<DiskWriter> diskWriter,
  EventTablePtr eventTable
):
EvBStateMachine(bu.get()),
bu_(bu),
fuProxy_(fuProxy),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
eventTable_(eventTable)
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


void evb::bu::StateMachine::buCache(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_CACHE";
  try
  {
    ruProxy_->superFragmentCallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
}


void evb::bu::StateMachine::buAllocate(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_ALLOCATE";
  try
  {
    fuProxy_->allocateI2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
}


void evb::bu::StateMachine::buDiscard(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_DISCARD";
  try
  {
    fuProxy_->discardI2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( Fail(sentinelException) );
  }
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
    if (doConfiguring_) stateMachine.fuProxy()->getApplicationDescriptors();
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
    if (doConfiguring_) stateMachine.fuProxy()->configure();
    if (doConfiguring_) stateMachine.diskWriter()->configure(maxEvtsUnderConstruction);
    if (doConfiguring_) stateMachine.eventTable()->configure(maxEvtsUnderConstruction);
    
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
    if (doClearing_) stateMachine.fuProxy()->clear();
    if (doClearing_) stateMachine.diskWriter()->clear();
    if (doClearing_) stateMachine.eventTable()->clear();
    
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

  stateMachine.bu()->resetMonitoringCounters();
  stateMachine.fuProxy()->resetMonitoringCounters();
  stateMachine.ruProxy()->resetMonitoringCounters();
  stateMachine.diskWriter()->resetMonitoringCounters();
  stateMachine.eventTable()->resetMonitoringCounters();
}


void evb::bu::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  const uint32_t runNumber = stateMachine.runNumber();

  stateMachine.diskWriter()->startProcessing(runNumber);
  stateMachine.bu()->startProcessing(runNumber);
  stateMachine.eventTable()->startProcessing();
}


void evb::bu::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.bu()->stopProcessing();
  stateMachine.eventTable()->stopProcessing();
  stateMachine.diskWriter()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
