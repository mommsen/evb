#include <math.h>
#include <string.h>

#include "evb/Constants.h"
#include "evb/readoutunit/ScalerHandler.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::readoutunit::ScalerHandler::ScalerHandler(const std::string& identifier) :
  dummyScalFedSize_(0),
  runNumber_(0),
  currentLumiSection_(0),
  doProcessing_(false),
  dataIsValid_(false)
{
  getFragmentPool(identifier);
  startRequestWorkloop(identifier);
}


void evb::readoutunit::ScalerHandler::getFragmentPool(const std::string& identifier)
{
  toolbox::net::URN urn("toolbox-mem-pool", identifier+"/ScalerFragmentPool");
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
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,new toolbox::mem::HeapAllocator());
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for scaler fragments", e);
  }
}


void evb::readoutunit::ScalerHandler::configure(const uint16_t fedId, const uint32_t dummyScalFedSize)
{
  fedId_ = fedId;
  dummyScalFedSize_ = dummyScalFedSize;
}


void evb::readoutunit::ScalerHandler::startProcessing(const uint32_t runNumber)
{
  if ( dummyScalFedSize_ == 0 ) return;

  runNumber_ = runNumber;

  dataIsValid_ = false;
  doProcessing_ = true;
  scalerRequestWorkLoop_->submit(scalerRequestAction_);

  while ( !dataIsValid_ ) ::usleep(1000);
}


void evb::readoutunit::ScalerHandler::stopProcessing()
{
  doProcessing_ = false;
}


toolbox::mem::Reference* evb::readoutunit::ScalerHandler::fillFragment
(
  const EvBid& evbId
)
{
  if ( dummyScalFedSize_ == 0 ) return 0;

  if ( evbId.lumiSection() > currentLumiSection_ )
  {
    dataForCurrentLumiSection_.swap(dataForNextLumiSection_);
    currentLumiSection_ = evbId.lumiSection();
    dataIsValid_ = false;
  }

  return getFerolFragment(evbId.eventNumber());
}


toolbox::mem::Reference* evb::readoutunit::ScalerHandler::getFerolFragment
(
  const uint32_t eventNumber
)
{
  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);
  const uint32_t dataSize = dataForCurrentLumiSection_.size();
  const uint32_t fedSize = dataSize + sizeof(fedh_t) + sizeof(fedt_t);

  if ( (fedSize & 0x7) != 0 )
  {
    std::ostringstream oss;
    oss << "The SCAL FED size of " << fedSize << " Bytes must be a multiple of 8 Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  uint16_t ferolBlocks = ceil( static_cast<double>(fedSize) / ferolPayloadSize );
  const uint32_t ferolSize = fedSize + ferolBlocks*sizeof(ferolh_t);
  const uint32_t frameSize = ferolSize + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  uint32_t dataOffset = 0;
  uint16_t crc(0xffff);

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,frameSize);

  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  memset(payload, 0, frameSize);
  bufRef->setDataSize(frameSize);

  I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  frame->partLength = ferolSize;
  frame->fedid = fedId_;
  frame->triggerno = eventNumber;
  payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

  for (uint16_t packetNumber = 0; packetNumber < ferolBlocks; ++packetNumber)
  {
    ferolh_t* ferolHeader = (ferolh_t*)payload;
    ferolHeader->set_signature();
    ferolHeader->set_fed_id(fedId_);
    ferolHeader->set_event_number(eventNumber);
    ferolHeader->set_packet_number(packetNumber);
    payload += sizeof(ferolh_t);
    uint32_t dataLength = 0;

    if (packetNumber == 0)
    {
      ferolHeader->set_first_packet();
      fedh_t* fedHeader = (fedh_t*)payload;
      fedHeader->sourceid = fedId_ << FED_SOID_SHIFT;
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
  }

  assert( dataOffset == dataForCurrentLumiSection_.size() );

  return bufRef;
}


void evb::readoutunit::ScalerHandler::startRequestWorkloop(const std::string& identifier)
{
  try
  {
    scalerRequestWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(identifier+"/scalerRequest", "waiting");

    if ( !scalerRequestWorkLoop_->isActive() )
      scalerRequestWorkLoop_->activate();

    scalerRequestAction_ =
      toolbox::task::bind(this,
        &evb::readoutunit::ScalerHandler::scalerRequest,
        identifier+"scalerRequestAction");
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start lumi accounting workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::readoutunit::ScalerHandler::scalerRequest(toolbox::task::WorkLoop* wl)
{
  requestLastestData();

  while ( doProcessing_ && dataIsValid_) ::sleep(1);

  return doProcessing_;
}


void evb::readoutunit::ScalerHandler::requestLastestData()
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


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
