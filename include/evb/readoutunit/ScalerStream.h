#ifndef _evb_readoutunit_ScalerStream_h_
#define _evb_readoutunit_ScalerStream_h_

#include <stdint.h>
#include <string.h>
#include <vector>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/FedFragment.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/HeapAllocator.h"
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
    * \brief Represent a stream for SCAL data
    */

    template<class ReadoutUnit, class Configuration>
    class ScalerStream : public FerolStream<ReadoutUnit,Configuration>, public toolbox::lang::Class
    {
    public:

      ScalerStream(ReadoutUnit*, const uint16_t fedId);

      /**
       * Append the next FED fragment to the super fragment
       */
      virtual void appendFedFragment(FragmentChainPtr&);

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);


    private:

      void createScalerPool();
      void startRequestWorkLoop();
      uint32_t getFedFragment(const EvBid&, FedFragmentPtr&);
      bool scalerRequest(toolbox::task::WorkLoop*);
      void requestLastestData();

      uint32_t dummyScalFedSize_;
      uint32_t currentLumiSection_;
      volatile bool dataIsValid_;

      toolbox::mem::Pool* fragmentPool_;
      toolbox::task::WorkLoop* scalerRequestWorkLoop_;
      toolbox::task::ActionSignature* scalerRequestAction_;

      CRCCalculator crcCalculator_;

      typedef std::vector<char> Buffer;
      Buffer dataForNextLumiSection_;
      Buffer dataForCurrentLumiSection_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::ScalerStream
(
  ReadoutUnit* readoutUnit,
  const uint16_t fedId
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,fedId),
  dummyScalFedSize_(this->configuration_->dummyScalFedSize),
  currentLumiSection_(0),
  dataIsValid_(false)
{
  createScalerPool();
  startRequestWorkLoop();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::createScalerPool()
{
  toolbox::net::URN urn("toolbox-mem-pool", this->readoutUnit_->getIdentifier("scalerFragmentPool"));

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
    toolbox::mem::HeapAllocator* a = new toolbox::mem::HeapAllocator();
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for scaler fragments", e);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::startRequestWorkLoop()
{
  try
  {
    scalerRequestWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("scalerRequest"), "waiting");

    if ( !scalerRequestWorkLoop_->isActive() )
      scalerRequestWorkLoop_->activate();

    scalerRequestAction_ =
      toolbox::task::bind(this,
                          &evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::scalerRequest,
                          this->readoutUnit_->getIdentifier("scalerRequestAction"));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start scaler request workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::scalerRequest(toolbox::task::WorkLoop* wl)
{
  requestLastestData();

  while ( this->doProcessing_ && dataIsValid_) ::sleep(1);

  return this->doProcessing_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::requestLastestData()
{
  if ( dataForNextLumiSection_.size() < dummyScalFedSize_ )
  {
    dataForNextLumiSection_.resize(dummyScalFedSize_);
  }

  // Simulate some dummy data changing every LS
  if ( currentLumiSection_ % 2 )
    memset(&dataForNextLumiSection_[0],0xef,dummyScalFedSize_);
  else
    memset(&dataForNextLumiSection_[0],0xa5,dummyScalFedSize_);

  dataIsValid_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::appendFedFragment(FragmentChainPtr& superFragment)
{
  const EvBid& evbId = superFragment->getEvBid();

  if ( evbId.lumiSection() > currentLumiSection_ )
  {
    dataForCurrentLumiSection_.swap(dataForNextLumiSection_);
    currentLumiSection_ = evbId.lumiSection();
    dataIsValid_ = false;
  }

  FedFragmentPtr fedFragment;
  const uint32_t fedSize = getFedFragment(evbId,fedFragment);

  this->updateInputMonitor(fedFragment,fedSize);
  this->maybeDumpFragmentToFile(fedFragment);

  superFragment->append(fedFragment);
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::getFedFragment
(
  const EvBid& evbId,
  FedFragmentPtr& fedFragment
)
{
  const uint32_t eventNumber = evbId.eventNumber();
  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);
  const uint32_t dataSize = dataForCurrentLumiSection_.size();
  const uint32_t fedSize = dataSize + sizeof(fedh_t) + sizeof(fedt_t);

  if ( (fedSize & 0x7) != 0 )
  {
    std::ostringstream oss;
    oss << "The SCAL FED " << this->fedId_ << " is " << fedSize << " Bytes, which is not a multiple of 8 Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  uint16_t ferolBlocks = ceil( static_cast<double>(fedSize) / ferolPayloadSize );
  assert(ferolBlocks < 2048);
  const uint32_t ferolSize = fedSize + ferolBlocks*sizeof(ferolh_t);
  const uint32_t frameSize = ferolSize + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  uint32_t dataOffset = 0;
  uint16_t crc(0xffff);

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,frameSize);

  bufRef->setDataSize(frameSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  memset(payload, 0, frameSize);

  I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  frame->totalLength = ferolSize;
  frame->fedid = this->fedId_;
  frame->triggerno = eventNumber;
  uint32_t partLength = 0;

  payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

  for (uint16_t packetNumber = 0; packetNumber < ferolBlocks; ++packetNumber)
  {
    ferolh_t* ferolHeader = (ferolh_t*)payload;
    ferolHeader->set_signature();
    ferolHeader->set_fed_id(this->fedId_);
    ferolHeader->set_event_number(eventNumber);
    ferolHeader->set_packet_number(packetNumber);
    payload += sizeof(ferolh_t);
    uint32_t dataLength = 0;

    if (packetNumber == 0)
    {
      ferolHeader->set_first_packet();
      fedh_t* fedHeader = (fedh_t*)payload;
      fedHeader->sourceid = this->fedId_ << FED_SOID_SHIFT;
      fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | eventNumber;
      payload += sizeof(fedh_t);
      dataLength += sizeof(fedh_t);
    }

    const uint32_t length = std::min(dataSize-dataOffset,ferolPayloadSize-dataLength);
    memcpy(payload,&dataForCurrentLumiSection_[0]+dataOffset,length);
    dataOffset += length;
    payload += length;
    dataLength += length;

    if (packetNumber == ferolBlocks-1)
    {
      ferolHeader->set_last_packet();
      fedt_t* fedTrailer = (fedt_t*)payload;
      payload += sizeof(fedt_t);
      dataLength += sizeof(fedt_t);

      fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) | (fedSize>>3);
      fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0x4);
      crcCalculator_.compute(crc,payload-dataLength,dataLength);
      fedTrailer->conscheck = (crc << FED_CRCS_SHIFT);
    }
    else
    {
      crcCalculator_.compute(crc,payload-dataLength,dataLength);
    }

    assert( dataLength <= ferolPayloadSize );
    ferolHeader->set_data_length(dataLength);
    partLength += dataLength;
  }

  frame->partLength = partLength;
  assert( dataOffset == dataForCurrentLumiSection_.size() );

  fedFragment.reset( new FedFragment(bufRef) );
  fedFragment->setEvBid(evbId);

  return fedSize;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::ScalerStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);

  if ( dummyScalFedSize_ == 0 ) return;

  dataIsValid_ = false;
  scalerRequestWorkLoop_->submit(scalerRequestAction_);

  while ( !dataIsValid_ ) ::usleep(1000);
}


#endif // _evb_readoutunit_ScalerStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
