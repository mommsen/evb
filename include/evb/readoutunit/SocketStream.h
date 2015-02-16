#ifndef _evb_readoutunit_SocketStream_h_
#define _evb_readoutunit_SocketStream_h_
 
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
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

//#define READER_USE_MALLOC
#define READER_USE_BUFFER


#include "evb/readoutunit/ferolTCP/Connection.h"
#include "evb/readoutunit/ferolTCP/Tools.h"
#include "evb/readoutunit/ferolTCP/BufferPool.h"
#include "evb/readoutunit/ferolTCP/FragmentBuffer.h"
#include "evb/readoutunit/ferolTCP/BufferedFragmentReader.h"


namespace evb {

  namespace readoutunit {

    
   /**
    * \ingroup xdaqApps
    * \brief Represent a stream of locally generated FEROL data
    */

    template<class ReadoutUnit, class Configuration>
    class SocketStream : public FerolStream<ReadoutUnit,Configuration>, public toolbox::lang::Class
    {
    public:

      SocketStream(ReadoutUnit*, const uint16_t fedId, ferolTCP::Connection* connection);

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


    private:

      void getFedFragment(uint16_t& fedId, uint32_t& eventNumber, unsigned char*& payload, size_t& length, ferolTCP::Buffer*& ferolTCPBuffer);
      void createFragmentPool(const std::string& fedId);
      void startRetrievingWorkLoop(const std::string& fedId);
      bool retrieving(toolbox::task::WorkLoop*);

      toolbox::task::WorkLoop* retrievingWorkLoop_;
      toolbox::task::ActionSignature* retrievingAction_;

      volatile bool retrieveFragments_;
      volatile bool retrievingActive_;

//      toolbox::mem::Pool* fragmentPool_;

      ferolTCP::ConnectionPtr ferolTCPConnection_;      
      ferolTCP::BufferPool ferolTCPBufferPool_; 
      ferolTCP::BufferedFragmentReaderPtr ferolTCPBufferedFragmentReader_;
    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

// Constructor is called during Configuring
template<class ReadoutUnit,class Configuration>
evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::SocketStream
(
  ReadoutUnit* readoutUnit,
  const uint16_t fedId,
  ferolTCP::Connection* connection
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,fedId),
  retrieveFragments_(false),
  retrievingActive_(false),
  ferolTCPConnection_(connection)
{
  const std::string fedIdStr = boost::lexical_cast<std::string>(this->fedId_);
  createFragmentPool(fedIdStr);
  startRetrievingWorkLoop(fedIdStr);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::createFragmentPool(const std::string& fedIdStr)
{	
	
#ifdef READER_USE_BUFFER
	
  #define BUF_SIZE 1024*1024
	  
  unsigned char* mallocBuffer1 = (unsigned char*) malloc(BUF_SIZE);
  unsigned char* mallocBuffer2 = (unsigned char*) malloc(BUF_SIZE);
  
  ferolTCP::Buffer* buffer1 = new ferolTCP::Buffer( mallocBuffer1, BUF_SIZE, ferolTCPBufferPool_ );
  ferolTCP::Buffer* buffer2 = new ferolTCP::Buffer( mallocBuffer2, BUF_SIZE, ferolTCPBufferPool_ );
  
  ferolTCPBufferPool_.addBuffer(buffer1);
  ferolTCPBufferPool_.addBuffer(buffer2);

  ferolTCP::FragmentBuffer* fragmentBuffer = new ferolTCP::FragmentBuffer( ferolTCPBufferPool_ );
		 
  ferolTCPBufferedFragmentReader_.reset( new ferolTCP::BufferedFragmentReader(*(ferolTCPConnection_.get()), *fragmentBuffer) );
  
#endif 
  
  //FIXME: A memory leak is created here because the buffers are not freed
  
  
//   toolbox::net::URN urn("toolbox-mem-pool", this->readoutUnit_->getIdentifier("fragmentPool_"+fedIdStr));
// 
//   try
//   {
//     toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
//   }
//   catch(toolbox::mem::exception::MemoryPoolNotFound)
//   {
//     // don't care
//   }
// 
//   try
//   {
//     toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(this->configuration_->fragmentPoolSize.value_);
//     fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
//   }
//   catch(toolbox::mem::exception::Exception& e)
//   {
//     XCEPT_RETHROW(exception::OutOfMemory,
//                   "Failed to create memory pool for dummy fragments", e);
//   }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startRetrievingWorkLoop(const std::string& fedIdStr)
{
  try
  {
    retrievingWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("retrieving_"+fedIdStr), "waiting");

    if ( !retrievingWorkLoop_->isActive() )
      retrievingWorkLoop_->activate();

    retrievingAction_ =
      toolbox::task::bind(this,
                          &evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::retrieving,
                          this->readoutUnit_->getIdentifier("retrievingAction_"+fedIdStr));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start retrieving workloop for FED " + fedIdStr;
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::retrieving(toolbox::task::WorkLoop*)
{
  retrievingActive_ = true;

  uint16_t fedId;
  uint32_t eventNumber;
  unsigned char* payload;
  size_t length;  
  ferolTCP::Buffer* ferolTCPBuffer;
  
  // Print thread ID
//  LOG4CPLUS_DEBUG( this->readoutUnit_->getApplicationLogger(), TCPFerolTools::getWorkLoopThreadID(retrievingWorkLoop_) );
  LOG4CPLUS_INFO( this->readoutUnit_->getApplicationLogger(), ferolTCP::tools::getWorkLoopThreadID(retrievingWorkLoop_) );

  try
  {
    while ( retrieveFragments_ )
    {
      // FIXME: FedFragment assumes that the whole FED fragment is in one buffer      

      getFedFragment(fedId, eventNumber, payload, length, ferolTCPBuffer);
      
      FedFragmentPtr fedFragment( new FedFragment(fedId, eventNumber, payload, length, ferolTCPBuffer) );

      this->addFedFragment(fedFragment);
    }
  }
  catch(xcept::Exception &e)
  {
    retrievingActive_ = false;
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    retrievingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    retrievingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  retrievingActive_ = false;

  return retrieveFragments_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::getFedFragment
(
  uint16_t& fedId, uint32_t& eventNumber, unsigned char*& payload, size_t& length, ferolTCP::Buffer*& ferolTCPBuffer
)
{
  
#ifdef READER_USE_BUFFER
  int received = ferolTCPBufferedFragmentReader_->receiveFragmentWithFerolHeadersBuffered(payload, fedId, eventNumber, ferolTCPBuffer);
#endif  
  
  
#ifdef READER_USE_MALLOC
  int bufSize = 4*1024;
  payload = (unsigned char*) malloc(bufSize);
  int received =  ferolTCPConnection_->receiveFragmentWithFerolHeaders(payload, bufSize, fedId, eventNumber);  
#endif

  
  if (received <= 0)
  {
    std::ostringstream oss;
    oss << "ERROR during receiveFragmentWithHeaders, returned size " << received;
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  length = received;
  
  return;



  /* Allocate here the bufRef and fill it with one FED fragment (incl FEROL headers) */

  // const uint32_t fedSize = fragmentTracker_.startFragment(evbId);

  // if ( (fedSize & 0x7) != 0 )
  // {
  //   std::ostringstream oss;
  //   oss << "The dummy FED " << this->fedId_ << " is " << fedSize << " Bytes, which is not a multiple of 8 Bytes";
  //   XCEPT_RAISE(exception::Configuration, oss.str());
  // }

  // const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);

  // // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  // const uint16_t ferolBlocks = (fedSize + ferolPayloadSize - 1)/ferolPayloadSize;
  // assert(ferolBlocks < 2048);
  // const uint32_t ferolSize = fedSize + ferolBlocks*sizeof(ferolh_t);
  // const uint32_t frameSize = ferolSize + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  // uint32_t remainingFedSize = fedSize;

  // try
  // {
  //   bufRef = toolbox::mem::getMemoryPoolFactory()->
  //     getFrame(fragmentPool_,frameSize);
  // }
  // catch(xcept::Exception)
  // {
  //   return false;
  // }

  // bufRef->setDataSize(frameSize);
  // unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  // memset(payload, 0, frameSize);
  // I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  // frame->totalLength = ferolSize;
  // frame->fedid = this->fedId_;
  // frame->triggerno = evbId.eventNumber();
  // uint32_t partLength = 0;

  // payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

  // for (uint16_t packetNumber=0; packetNumber < ferolBlocks; ++packetNumber)
  // {
  //   ferolh_t* ferolHeader = (ferolh_t*)payload;
  //   ferolHeader->set_signature();
  //   ferolHeader->set_fed_id(this->fedId_);
  //   ferolHeader->set_event_number(evbId.eventNumber());
  //   ferolHeader->set_packet_number(packetNumber);
  //   payload += sizeof(ferolh_t);
  //   uint32_t length = 0;

  //   if (packetNumber == 0)
  //     ferolHeader->set_first_packet();

  //   if ( remainingFedSize > ferolPayloadSize )
  //   {
  //     length = ferolPayloadSize;
  //   }
  //   else
  //   {
  //     length = remainingFedSize;
  //     ferolHeader->set_last_packet();
  //   }
  //   remainingFedSize -= length;

  //   const size_t filledBytes = fragmentTracker_.fillData(payload, length);
  //   ferolHeader->set_data_length(filledBytes);

  //   payload += filledBytes;
  //   partLength += filledBytes + sizeof(ferolh_t);
  // }

  // frame->partLength = partLength;
  // assert( remainingFedSize == 0 );

  // return true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);

  retrieveFragments_ = true;
  retrievingWorkLoop_->submit(retrievingAction_);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::drain()
{
  retrieveFragments_ = false;
  while ( retrievingActive_ ) ::usleep(1000);
  this->fragmentFIFO_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::stopProcessing()
{
  retrieveFragments_ = false;

  FerolStream<ReadoutUnit,Configuration>::stopProcessing();
}


#endif // _evb_readoutunit_SocketStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
