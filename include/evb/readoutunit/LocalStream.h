#ifndef _evb_readoutunit_LocalStream_h_
#define _evb_readoutunit_LocalStream_h_

#include <stdint.h>
#include <string.h>

#include "evb/EvBid.h"
#include "evb/FragmentTracker.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "interface/shared/ferol_header.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
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
      virtual void setMaxTriggerRate(const uint32_t maxTriggerRate);


    private:

      bool getFedFragment(const EvBid&,toolbox::mem::Reference*&);
      void createFragmentPool(const std::string& fedId);
      void startGeneratorWorkLoop(const std::string& fedId);
      bool generating(toolbox::task::WorkLoop*);

      toolbox::task::WorkLoop* generatingWorkLoop_;
      toolbox::task::ActionSignature* generatingAction_;

      volatile bool generatingActive_;

      FragmentTracker fragmentTracker_;
      toolbox::mem::Pool* fragmentPool_;

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
  generatingActive_(false),
  fragmentTracker_(fedId,
                   this->configuration_->dummyFedSize,
                   this->configuration_->useLogNormal,
                   this->configuration_->dummyFedSizeStdDev,
                   this->configuration_->dummyFedSizeMin,
                   this->configuration_->dummyFedSizeMax,
                   this->configuration_->computeCRC)
{
  const std::string fedIdStr = boost::lexical_cast<std::string>(this->fedId_);
  createFragmentPool(fedIdStr);
  startGeneratorWorkLoop(fedIdStr);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::createFragmentPool(const std::string& fedIdStr)
{
  toolbox::net::URN urn("toolbox-mem-pool", this->readoutUnit_->getIdentifier("fragmentPool_"+fedIdStr));

  try
  {
    toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
  }
  catch(toolbox::mem::exception::MemoryPoolNotFound)
  {
    // don't care
  }

  try
  {
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(this->configuration_->fragmentPoolSize.value_);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for dummy fragments", e);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::startGeneratorWorkLoop(const std::string& fedIdStr)
{
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
  toolbox::mem::Reference* bufRef = 0;
  fragmentTracker_.startRun();
  EvBid evbId = this->evbIdFactory_.getEvBid();

  try
  {
    while ( evbId.eventNumber() <= this->eventNumberToStop_ )
    {
      if ( !this->fragmentFIFO_.full() && getFedFragment(evbId,bufRef) )
      {
        FedFragmentPtr fedFragment = this->fedFragmentFactory_.getFedFragment(this->fedId_,evbId,bufRef);
        this->addFedFragmentWithEvBid(fedFragment);
        evbId = this->evbIdFactory_.getEvBid();
      }
      else
      {
        ::usleep(10);
      }
    }
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
bool evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::getFedFragment
(
  const EvBid& evbId,
  toolbox::mem::Reference*& bufRef
)
{
  const uint32_t fedSize = fragmentTracker_.startFragment(evbId);

  if ( (fedSize & 0x7) != 0 )
  {
    std::ostringstream oss;
    oss << "The dummy FED " << this->fedId_ << " is " << fedSize << " Bytes, which is not a multiple of 8 Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);

  // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  const uint16_t ferolBlocks = (fedSize + ferolPayloadSize - 1)/ferolPayloadSize;
  assert(ferolBlocks < 2048);
  const uint32_t ferolSize = fedSize + ferolBlocks*sizeof(ferolh_t);
  uint32_t remainingFedSize = fedSize;

  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
      getFrame(fragmentPool_,ferolSize);
  }
  catch(xcept::Exception)
  {
    return false;
  }

  bufRef->setDataSize(ferolSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  memset(payload, 0, ferolSize);

  for (uint16_t packetNumber=0; packetNumber < ferolBlocks; ++packetNumber)
  {
    ferolh_t* ferolHeader = (ferolh_t*)payload;
    ferolHeader->set_signature();
    ferolHeader->set_fed_id(this->fedId_);
    ferolHeader->set_event_number(evbId.eventNumber());
    ferolHeader->set_packet_number(packetNumber);
    payload += sizeof(ferolh_t);
    uint32_t length = 0;

    if (packetNumber == 0)
      ferolHeader->set_first_packet();

    if ( remainingFedSize > ferolPayloadSize )
    {
      length = ferolPayloadSize;
    }
    else
    {
      length = remainingFedSize;
      ferolHeader->set_last_packet();
    }
    remainingFedSize -= length;

    const size_t filledBytes = fragmentTracker_.fillData(payload, length);
    ferolHeader->set_data_length(filledBytes);

    payload += filledBytes;
  }

  assert( remainingFedSize == 0 );

  return true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);

  this->eventNumberToStop_ = (1 << 25); //larger than maximum event number
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


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::setMaxTriggerRate(const uint32_t maxTriggerRate)
{
  fragmentTracker_.setMaxTriggerRate(maxTriggerRate);
}


#endif // _evb_readoutunit_LocalStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
