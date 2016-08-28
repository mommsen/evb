#ifndef _evb_readoutunit_LocalStream_h_
#define _evb_readoutunit_LocalStream_h_

#include <stdint.h>
#include <string.h>

#include "evb/Constants.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "toolbox/lang/Class.h"
#include "toolbox/math/random.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Represent a stream of locally generated FEROL data
    */

    template<class ReadoutUnit, class Configuration>
    class LocalStream : public FerolStream<ReadoutUnit,Configuration>, public toolbox::lang::Class
    {
    public:

      LocalStream(ReadoutUnit*, const uint16_t fedId);

      ~LocalStream();

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);

      /**
       * Drain the remainig events
       */
      virtual void drain();

      /**
       * Stop processing events
       */
      virtual void stopProcessing();

      /**
       * Set the trigger rate for generating events
       */
      virtual void setMaxTriggerRate(const uint32_t maxTriggerRate)
      { maxTriggerRate_ = maxTriggerRate; }


    private:

      void startGeneratorWorkLoop();
      bool generating(toolbox::task::WorkLoop*);
      void waitForNextTrigger();
      uint32_t getFedSize() const;

      const boost::shared_ptr<Configuration> configuration_;
      toolbox::task::WorkLoop* generatingWorkLoop_;
      toolbox::task::ActionSignature* generatingAction_;
      boost::scoped_ptr<toolbox::math::LogNormalGen> logNormalGen_;

      volatile bool generatingActive_;
      uint32_t maxTriggerRate_;
      uint64_t lastTime_;
      uint32_t availableTriggers_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::LocalStream
(
  ReadoutUnit* readoutUnit,
  const uint16_t fedId
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,fedId),
  configuration_(readoutUnit->getConfiguration()),
  generatingActive_(false),
  maxTriggerRate_(0),
  lastTime_(0),
  availableTriggers_(0)
{
  if ( configuration_->dummyFedSizeMin.value_ < sizeof(fedh_t) + sizeof(fedt_t) )
  {
    std::ostringstream msg;
    msg << "The minimal FED size in the configuration (dummyFedSizeMin) must be at least ";
    msg << sizeof(fedh_t) + sizeof(fedt_t) << " Bytes instead of " << configuration_->dummyFedSizeMin << " Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  if ( configuration_->useLogNormal )
  {
    logNormalGen_.reset( new toolbox::math::LogNormalGen(getTimeStamp(),configuration_->dummyFedSize,configuration_->dummyFedSizeStdDev) );
  }

  startGeneratorWorkLoop();
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::~LocalStream()
{
  if ( generatingWorkLoop_ && generatingWorkLoop_->isActive() )
    generatingWorkLoop_->cancel();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::startGeneratorWorkLoop()
{
  const std::string fedIdStr = boost::lexical_cast<std::string>(this->fedId_);
  try
  {
    generatingWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("generating_"+fedIdStr), "waiting");

    if ( !generatingWorkLoop_->isActive() )
      generatingWorkLoop_->activate();

    generatingAction_ =
      toolbox::task::bind(this,
                          &evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::generating,
                          this->readoutUnit_->getIdentifier("generatingAction_"+fedIdStr));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start event generation workloop for FED " + fedIdStr;
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::generating(toolbox::task::WorkLoop*)
{
  generatingActive_ = true;
  FedFragmentPtr fedFragment;

  try
  {
    do
    {
      waitForNextTrigger();
      const uint32_t fedSize = getFedSize();
      fedFragment = this->fedFragmentFactory_.getDummyFragment(this->fedId_,this->isMasterStream_,fedSize,
                                                               configuration_->computeCRC);
      this->addFedFragment(fedFragment);
    }
    while ( fedFragment->getEventNumber() < this->eventNumberToStop_ );
  }
  catch(xcept::Exception &e)
  {
    generatingActive_ = false;
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    generatingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    generatingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  generatingActive_ = false;

  return false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::waitForNextTrigger()
{
  if ( maxTriggerRate_ == 0 ) return;

  if ( availableTriggers_ > 0 )
  {
    --availableTriggers_;
    return;
  }

  uint64_t now = 0;
  while ( availableTriggers_ == 0 )
  {
    now = getTimeStamp();
    if ( lastTime_ == 0 )
      availableTriggers_ = 1;
    else
      availableTriggers_ = static_cast<uint32_t>(now>lastTime_ ? (now-lastTime_)/1e9*maxTriggerRate_ : 0);
  }
  lastTime_ = now;
  --availableTriggers_;
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::getFedSize() const
{
  uint32_t fedSize;
  if ( configuration_->useLogNormal )
  {
    fedSize = std::max((uint32_t)logNormalGen_->getRawRandomSize(), configuration_->dummyFedSizeMin.value_);
    if ( configuration_->dummyFedSizeMax.value_ > 0 && fedSize > configuration_->dummyFedSizeMax.value_ )
      fedSize = configuration_->dummyFedSizeMax;
  }
  else
  {
    fedSize = configuration_->dummyFedSize;
  }
  return fedSize & ~0x7;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);

  this->eventNumberToStop_ = (1 << 25); //larger than maximum event number
  lastTime_ = getTimeStamp();
  availableTriggers_ = 0;

  generatingWorkLoop_->submit(generatingAction_);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::drain()
{
  while ( generatingActive_ ) ::usleep(1000);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::stopProcessing()
{
  this->eventNumberToStop_ = 0;

  FerolStream<ReadoutUnit,Configuration>::stopProcessing();
}


#endif // _evb_readoutunit_LocalStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
