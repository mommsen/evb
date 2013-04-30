#include "i2o/Method.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/CRC16.h"
#include "evb/EventUtils.h"
#include "evb/Exception.h"
#include "evb/FragmentGenerator.h"
#include "evb/FragmentTracker.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"

#include <assert.h>
#include <byteswap.h>
#include <math.h>
#include <sstream>
#include <sys/time.h>

evb::FragmentGenerator::FragmentGenerator() :
frameSize_(0),
fedSize_(0),
eventNumber_(1),
usePlayback_(false)
{
  reset();
}


void evb::FragmentGenerator::configure
(
  const uint32_t fedId,
  const bool usePlayback,
  const std::string& playbackDataFile,
  const uint32_t frameSize,
  const uint32_t fedSize,
  const uint32_t fedSizeStdDev,
  const size_t fragmentPoolSize
)
{
  if (fedId > FED_SOID_WIDTH)
  {
    std::stringstream oss;
    
    oss << "fedSourceId is too large.";
    oss << "Actual value: " << fedId;
    oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  fedId_ = fedId;

  if ( fedSize % 8 != 0 )
  {
    std::stringstream oss;
    
    oss << "The requested FED payload of " << fedSize << " bytes";
    oss << " is not a multiple of 8 bytes";
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  frameSize_ = frameSize;
  fedSize_ = fedSize;
  usePlayback_ = usePlayback;

  toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
  try
  {
    toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    // don't care
  }
  
  try
  {
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(fragmentPoolSize);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for dummy fragments.", e);
  }
  
  fragmentTracker_.reset( 
    new FragmentTracker(fedId,fedSize,fedSizeStdDev)
  );
  
  playbackData_.clear();
  if ( usePlayback )
    cacheData(playbackDataFile);
}


void evb::FragmentGenerator::cacheData(const std::string& playbackDataFile)
{
  toolbox::mem::Reference* bufRef = 0;
  fillData(bufRef);
  playbackData_.push_back(bufRef);
  playbackDataPos_ = playbackData_.begin();
}


void evb::FragmentGenerator::reset()
{
  playbackDataPos_ = playbackData_.begin();
  eventNumber_ = 1;
}


bool evb::FragmentGenerator::getData(toolbox::mem::Reference*& bufRef)
{
  if ( usePlayback_ )
  {
    // bufRef = clone(*playbackDataPos_);
    // if ( bufRef == 0 ) return false;
    
    if ( (*playbackDataPos_)->getBuffer()->getRefCounter() > 1024 ) return false;
    bufRef = (*playbackDataPos_)->duplicate();
    
    if ( ++playbackDataPos_ == playbackData_.end() )
      playbackDataPos_ = playbackData_.begin();
    return true;
  }
  else
  {
    return fillData(bufRef);
  }
}


bool evb::FragmentGenerator::fillData(toolbox::mem::Reference*& bufRef)
{
  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fragmentPool_,frameSize_);
  }
  catch(xcept::Exception& e)
  {
    return false;
  }
  
  unsigned char* frame = (unsigned char*)bufRef->getDataLocation();
  bzero(bufRef->getDataLocation(), bufRef->getBuffer()->getSize());
  
  uint32_t usedFrameSize = 0;
  uint32_t remainingFedSize = fragmentTracker_->startFragment(eventNumber_);
  const uint32_t ferolBlockSize = 4*1024;
  
  while ( (usedFrameSize + remainingFedSize + sizeof(ferolh_t)) <= bufRef->getBuffer()->getSize() )
  {
    uint32_t packetNumber = 0;

    while ( remainingFedSize > 0 )
    {
      assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
      uint32_t length;
      
      ferolh_t* ferolHeader = (ferolh_t*)frame;
      ferolHeader->set_signature();
      ferolHeader->set_packet_number(packetNumber);
      
      if (packetNumber == 0)
        ferolHeader->set_first_packet();
      
      if ( remainingFedSize > ferolBlockSize )
      {
        length = ferolBlockSize;
      }
      else
      {
        length = remainingFedSize;
        ferolHeader->set_last_packet();
      }
      remainingFedSize -= length;
      frame += sizeof(ferolh_t);
      
      const size_t filledBytes = fragmentTracker_->fillData(frame, length);
      //const size_t filledBytes = length;
      
      ferolHeader->set_data_length(filledBytes);
      ferolHeader->set_fed_id(fedId_);
      ferolHeader->set_event_number(eventNumber_);
      
      // assert( ferolHeader->signature() == FEROL_SIGNATURE );
      // assert( ferolHeader->packet_number() == packetNumber );
      // assert( ferolHeader->data_length() == filledBytes );
      // assert( ferolHeader->data_length() == length );
      // assert( ferolHeader->fed_id() == fedId_ );
      // assert( ferolHeader->event_number() == eventNumber_ );
      
      frame += filledBytes;
      usedFrameSize += filledBytes + sizeof(ferolh_t);
      
      ++packetNumber;
      assert(packetNumber < 2048);
    }
    
    if (++eventNumber_ % (1 << 24) == 0) eventNumber_ = 1;
    remainingFedSize = fragmentTracker_->startFragment(eventNumber_);
  }
  
  bufRef->setDataSize(usedFrameSize);
  
  return true;
}


toolbox::mem::Reference* evb::FragmentGenerator::clone
(
  toolbox::mem::Reference* bufRef
) const
{
  toolbox::mem::Reference* head = 0;
  toolbox::mem::Reference* tail = 0;
  toolbox::mem::Reference* copyBufRef = 0;
  
  while (bufRef)
  {
    try
    {
      copyBufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fragmentPool_, bufRef->getDataSize());
    }
    catch(xcept::Exception& e)
    {
      if (head) head->release();
      return 0;
    }

    memcpy(
      (char*)copyBufRef->getDataLocation(),
      bufRef->getDataLocation(),
      bufRef->getDataSize()
    );
    
    // Set the size of the copy
    copyBufRef->setDataSize(bufRef->getDataSize());

    if (tail)
    {
      tail->setNextReference(copyBufRef);
      tail = copyBufRef;
    }
    else
    {
      head = copyBufRef;
      tail = copyBufRef;
    }
    bufRef = bufRef->getNextReference();
  }
  return head;
}


void evb::FragmentGenerator::fillTriggerPayload
(
  unsigned char* fedPtr,
  const uint32_t eventNumber,
  const L1Information& l1Info
) const
{
  using namespace evtn;
  
  //set offsets based on record scheme 
  evm_board_setformat(fedSize_);
  
  unsigned char* ptr = fedPtr + sizeof(fedh_t);

  //board id
  unsigned char *pptr = ptr + EVM_BOARDID_OFFSET * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = (EVM_BOARDID_VALUE << EVM_BOARDID_SHIFT);

  //setup version
  pptr = ptr + EVM_GTFE_SETUPVERSION_OFFSET * SLINK_HALFWORD_SIZE;
  if (EVM_GTFE_BLOCK == EVM_GTFE_BLOCK_V0011)
    *((uint32_t*)(pptr)) = 0xffffffff & EVM_GTFE_SETUPVERSION_MASK;
  else
    *((uint32_t*)(pptr)) = 0x00000000 & EVM_GTFE_SETUPVERSION_MASK;

  //fdl mode
  pptr = ptr + EVM_GTFE_FDLMODE_OFFSET * SLINK_HALFWORD_SIZE;
  if (EVM_FDL_NOBX == 5)
    *((uint32_t*)(pptr)) = 0xffffffff & EVM_GTFE_FDLMODE_MASK;
  else
    *((uint32_t*)(pptr)) = 0x00000000 & EVM_GTFE_FDLMODE_MASK;

  //gps time
  timeval tv;
  gettimeofday(&tv,0);
  pptr = ptr + EVM_GTFE_BSTGPS_OFFSET * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = tv.tv_usec;
  pptr += SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = tv.tv_sec;

  //TCS chip id
  pptr = ptr + (EVM_GTFE_BLOCK*2 + EVM_TCS_BOARDID_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = ((EVM_TCS_BOARDID_VALUE << EVM_TCS_BOARDID_SHIFT) & EVM_TCS_BOARDID_MASK);

  //event number
  pptr = ptr + (EVM_GTFE_BLOCK*2 + EVM_TCS_TRIGNR_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = eventNumber;

  //orbit number
  pptr = ptr + (EVM_GTFE_BLOCK*2 + EVM_TCS_ORBTNR_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = l1Info.orbitNumber;

  //lumi section
  pptr = ptr + (EVM_GTFE_BLOCK*2 + EVM_TCS_LSBLNR_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = l1Info.lsNumber + ((l1Info.eventType << EVM_TCS_EVNTYP_SHIFT) & EVM_TCS_EVNTYP_MASK);

  // bunch crossing in fdl bx+0 (-1,0,1) for nbx=3 i.e. offset by one full FDB block and leave -1/+1 alone (it will be full of zeros)
  // add also TCS chip Id
  pptr = ptr + ((EVM_GTFE_BLOCK + EVM_TCS_BLOCK + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2))*2 + EVM_FDL_BCNRIN_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint32_t*)(pptr)) = (l1Info.bunchCrossing & EVM_TCS_BCNRIN_MASK) + ((EVM_FDL_BOARDID_VALUE << EVM_FDL_BOARDID_SHIFT) & EVM_FDL_BOARDID_MASK);

  // tech trig 64-bit set
  pptr = ptr + ((EVM_GTFE_BLOCK + EVM_TCS_BLOCK + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2))*2 + EVM_FDL_TECTRG_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint64_t*)(pptr)) = l1Info.l1Technical;
  pptr = ptr + ((EVM_GTFE_BLOCK + EVM_TCS_BLOCK + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2))*2 + EVM_FDL_ALGOB1_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint64_t*)(pptr)) = l1Info.l1Decision_0_63;
  pptr = ptr + ((EVM_GTFE_BLOCK + EVM_TCS_BLOCK + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2))*2 + EVM_FDL_ALGOB2_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint64_t*)(pptr)) = l1Info.l1Decision_64_127;

  // prescale version is 0
  pptr = ptr + ((EVM_GTFE_BLOCK + EVM_TCS_BLOCK + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2))*2 + EVM_FDL_PSCVSN_OFFSET) * SLINK_HALFWORD_SIZE;
  *((uint64_t*)(pptr)) = 0;
}


void evb::FragmentGenerator::updateCRC
(
  const unsigned char* fedPtr
) const
{
  fedt_t* fedTrailer = (fedt_t*)(fedPtr + fedSize_ - sizeof(fedt_t));
  
  // Force CRC field to zero before re-computing the CRC.
  // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
  fedTrailer->conscheck = 0;
  
  unsigned short crc = compute_crc(fedPtr,fedSize_);
  fedTrailer->conscheck = (crc << FED_CRCS_SHIFT);
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
