#include "i2o/Method.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/frl_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/CRC16.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/EventUtils.h"
#include "evb/Exception.h"
#include "evb/SuperFragmentGenerator.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"

#include <assert.h>
#include <math.h>
#include <sstream>
#include <sys/time.h>

evb::SuperFragmentGenerator::SuperFragmentGenerator(const std::string& poolName) :
dummyBlockSize_(0),
dummyFedPayloadSize_(0),
eventNumber_(1),
fedCRC_(0),
usePlayback_(false)
{
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", "udapl");
    dummySuperFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    toolbox::net::URN urn("toolbox-mem-pool", poolName);
    try
    {
      dummySuperFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        createPool(urn, new toolbox::mem::HeapAllocator());
    }
    catch (toolbox::mem::exception::DuplicateMemoryPool)
    {
      // Pool already exists from a previous construction of this class
      // Note that destroying the pool in the destructor is not working
      // because the previous instance is destroyed after the new one
      // is constructed.
      dummySuperFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
      findPool(urn);
    }
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for dummy super-fragments.", e);
  }

  reset();
}


void evb::SuperFragmentGenerator::configure
(
  const xdata::Vector<xdata::UnsignedInteger32>& fedSourceIds,
  const bool usePlayback,
  const std::string& playbackDataFile,
  const uint32_t dummyBlockSize,
  const uint32_t dummyFedPayloadSize,
  const uint32_t dummyFedPayloadStdDev,
  const uint32_t maxFragmentsInMemory
)
{
  if ( fedSourceIds.empty() && !usePlayback )
  {
    XCEPT_RAISE(exception::Configuration,
      "A dummy super-fragment must have at least one source FED");
  }
  
  fedSourceIds_.clear();
  fedSourceIds_.reserve(fedSourceIds.size());
  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = fedSourceIds.begin(), itEnd = fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint32_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::stringstream oss;
      
      oss << "fedSourceId is too large.";
      oss << "Actual value: " << fedId;
      oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    fedSourceIds_.push_back(fedId);
  }

  if ( dummyFedPayloadSize % 8 != 0 )
  {
    std::stringstream oss;
    
    oss << "The requested FED payload of " << dummyFedPayloadSize << " bytes";
    oss << " is not a multiple of 8 bytes";
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  superFragmentTracker_.reset( new SuperFragmentTracker
    (dummyFedPayloadSize,dummyFedPayloadStdDev)
  );
  dummyBlockSize_ = dummyBlockSize;
  dummyFedPayloadSize_ = dummyFedPayloadSize;

  usePlayback_ = usePlayback;

  playbackData_.clear();
  playbackDataPos_ = playbackData_.begin();
  if ( usePlayback )
    cacheData(playbackDataFile);

  if ( maxFragmentsInMemory > 0 )
  {
    const size_t payload =
      sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) + // RU builder header
      sizeof(frlh_t)                             + // FRL header
      sizeof(fedh_t)                             + // FED header
      dummyFedPayloadSize                        + // FED payload
      sizeof(fedt_t);                              // FED trailer
    
    dummySuperFragmentPool_->setHighThreshold(maxFragmentsInMemory*fedSourceIds_.size()*payload);
  }
}


void evb::SuperFragmentGenerator::reset()
{
  playbackDataPos_ = playbackData_.begin();
  eventNumber_ = 1;
  evbIdFactory_.reset();
}


bool evb::SuperFragmentGenerator::getData(toolbox::mem::Reference*& bufRef)
{
  if ( usePlayback_ )
  {
    if ( playbackData_.end() == ++playbackDataPos_ ) playbackDataPos_ = playbackData_.begin();
    bufRef = clone(playbackDataPos_->second);
    return true;
  }
  else
  {
    EvBid evbId = evbIdFactory_.getEvBid(eventNumber_);
    if ( getSuperFragment(bufRef,evbId) )
    {
      // Increment the event number, which is 24-bits and starts with 1
      if (++eventNumber_ % (1 << 24) == 0) eventNumber_ = 1;
      return true;
    }
  }
  return false;
}


bool evb::SuperFragmentGenerator::getData
(
  toolbox::mem::Reference*& bufRef,
  const EvBid& evbId
)
{
  if ( usePlayback_ )
    return getFragmentFromPlayback(bufRef,evbId);
  else
    return getSuperFragment(bufRef,evbId);
}


bool evb::SuperFragmentGenerator::getData
(
  toolbox::mem::Reference*& bufRef,
  const EvBid& evbId,
  const L1Information& l1Info
)
{
  if ( usePlayback_ )
    return getFragmentFromPlayback(bufRef,evbId);

  if ( ! getSuperFragment(bufRef,evbId) ) return false;

  unsigned char* fedPtr = (unsigned char*)bufRef->getDataLocation()
    + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME)
    + sizeof(frlh_t);
    
  fillTriggerPayload(fedPtr,evbId.eventNumber(),l1Info);

  updateCRC(fedPtr);

  return true;
}


bool evb::SuperFragmentGenerator::getFragmentFromPlayback
(
  toolbox::mem::Reference*& bufRef,
  const EvBid& evbId
)
{
  // playbackDataPos_ = playbackData_.find(evbId);
  // if ( playbackData_.end() == playbackDataPos_ )
  // {
  //   playbackDataPos_ = playbackData_.begin();
  //   return false;
  // }
  // bufRef = playbackDataPos_->second;

  bufRef = clone(playbackData_.begin()->second);
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  block->resyncCount = evbId.resyncCount();

  return true;
}


toolbox::mem::Reference* evb::SuperFragmentGenerator::clone
(
  toolbox::mem::Reference* bufRef
) const
{
  toolbox::mem::Reference* head = 0;
  toolbox::mem::Reference* tail = 0;
  
  while (bufRef)
  {
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(dummySuperFragmentPool_, bufRef->getDataSize());
    
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


size_t evb::SuperFragmentGenerator::getMemoryUsage() const
{
  if (dummySuperFragmentPool_)
    return dummySuperFragmentPool_->getMemoryUsage().getUsed();
  else
    return 0;
}


void evb::SuperFragmentGenerator::cacheData(const std::string& playbackDataFile)
{
  toolbox::mem::Reference* bufRef = 0;
  EvBid evbId = evbIdFactory_.getEvBid(1);
  getSuperFragment(bufRef,evbId);
  playbackData_.insert(PlaybackData::value_type(evbId,bufRef));
}


bool evb::SuperFragmentGenerator::getSuperFragment
(
  toolbox::mem::Reference*& bufRef,
  const EvBid& evbId
)
{
  if ( dummySuperFragmentPool_->isHighThresholdExceeded() ) return false;

  toolbox::mem::Reference* head = 0;
  toolbox::mem::Reference* tail = 0;
  
  superFragmentTracker_->startSuperFragment(fedSourceIds_);
  uint16_t blockNb = 0;
  
  while (!superFragmentTracker_->reachedEndOfSuperFragment())
  {
    // Allocate memory for a super-fragment block
    toolbox::mem::Reference* nextBlock = 0;
    try
    {
       nextBlock = toolbox::mem::getMemoryPoolFactory()->
        getFrame(dummySuperFragmentPool_,dummyBlockSize_);
    }
    catch(toolbox::mem::exception::Exception& e)
    {
      return false;
    }
    catch(xcept::Exception& e)
    {
      XCEPT_RETHROW(exception::OutOfMemory,
        "Failed to allocate memory for super-fragment block", e);
    }
    
    fillBlock(nextBlock,blockNb,evbId);
    
    // Append block to super-fragment
    if (head == 0)
    {
      head = nextBlock;
      tail = nextBlock;
    }
    else
    {
      tail->setNextReference(nextBlock);
      tail = nextBlock;
    }
    
    blockNb++;
  }
  
  setNbBlocksInSuperFragment(head,blockNb);

  bufRef = head;
  return true;
}


void evb::SuperFragmentGenerator::fillBlock
(
  toolbox::mem::Reference* bufRef,
  const uint16_t blockNb,
  const EvBid& evbId
)
{
  ///////////////////////////////////////////////////////////
  // Calculate addresses of block, FRL header and FED data //
  ///////////////////////////////////////////////////////////
  
  unsigned char* blockAddr = (unsigned char*)bufRef->getDataLocation();
  size_t nbFedBytesWritten = 0;

  /////////////////////
  // Insert FED data //
  /////////////////////
  
  unsigned char* pos = blockAddr + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) + sizeof(frlh_t);
  size_t nbFreeBytes =
    dummyBlockSize_ - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) - sizeof(frlh_t);
  
  while ( !superFragmentTracker_->reachedEndOfSuperFragment() )
  {
    SuperFragmentTracker::FedComponentDescriptor component =
      superFragmentTracker_->getNextComponent(nbFreeBytes);

    // Stop filling block if not enough free space for next component
    if (nbFreeBytes < component.size)
    {
      break;
    };
    
    try
    {
      insertFedComponent(component,pos,evbId.eventNumber());
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::OutOfMemory,
        "Failed to insert FED component", e);
    }
    
    // Update position within super-fragment
    superFragmentTracker_->moveToNextComponent(nbFreeBytes);
    
    // Update position within block
    pos += component.size;
    nbFreeBytes -= component.size;
    
    // Update number of FED bytes written
    nbFedBytesWritten += component.size;
  }
  
  const bool superFragmentComplete = superFragmentTracker_->reachedEndOfSuperFragment();
  
  // I2O message size in bytes
  const size_t i2oMessageSize = sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
    sizeof(frlh_t) + nbFedBytesWritten;
  
  fillRuAndFrlHeaders(
    blockAddr,
    i2oMessageSize,
    nbFedBytesWritten,
    evbId,
    blockNb,
    superFragmentComplete
  );
  
  bufRef->setDataSize(i2oMessageSize);
}


void evb::SuperFragmentGenerator::insertFedComponent
(
  const SuperFragmentTracker::FedComponentDescriptor& component,
  const unsigned char* startAddr,
  const uint32_t eventNumber
)
{
  fedh_t* fedHeader = 0;
  fedt_t* fedTrailer = 0;
  size_t bufSize = 0;
  
  switch (component.type)
  {
    case SuperFragmentTracker::FED_HEADER:
      fedHeader = (fedh_t*)startAddr;
      
      fedHeader->sourceid = component.fedId << FED_SOID_SHIFT;
      fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | eventNumber;
      
      fedCRC_ = evf::compute_crc(startAddr,sizeof(fedh_t));
      
      break;
      
    case SuperFragmentTracker::FED_PAYLOAD:
      assert( component.size % 8 == 0 );
      bufSize = component.size / 8;
      for (size_t i=0; i<bufSize; ++i)
        fedCRC_ = evf::compute_crc_64bit(fedCRC_,&startAddr[i*8]);
      
      break;
      
    case SuperFragmentTracker::FED_TRAILER:
      fedTrailer = (fedt_t*)startAddr;
      
      fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) |
        ((sizeof(fedh_t) + superFragmentTracker_->getFedPayloadSize() + sizeof(fedt_t)) >> 3);
      
      // Force CRC field to zero before re-computing the CRC.
      // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
      fedTrailer->conscheck = 0;
      
      bufSize = sizeof(fedt_t) / 8;
      for (size_t i=0; i<bufSize; ++i) 
        fedCRC_ = evf::compute_crc_64bit(fedCRC_,&startAddr[i*8]);
      
      fedTrailer->conscheck = (fedCRC_ << FED_CRCS_SHIFT);
      
      break;
      
    default:
      XCEPT_RAISE(exception::SuperFragment,
        "Unknown type of FED data component");
  }
}


void evb::SuperFragmentGenerator::fillRuAndFrlHeaders
(
  const unsigned char* blockAddr,
  const size_t i2oMessageSize,
  const size_t nbFedBytesWritten,
  const EvBid& evbId,
  const uint16_t blockNb,
  const bool isLastBlockOfSuperFragment
) const
{
  const unsigned char* frlHeaderAddr = blockAddr+sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  
  I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)blockAddr;
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)blockAddr;
  frlh_t* frlHeader = (frlh_t*)frlHeaderAddr;
  
  stdMsg->MessageSize      = i2oMessageSize >> 2;
  stdMsg->InitiatorAddress = 0;
  stdMsg->TargetAddress    = 0;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;  // Point-to-point
  
  block->eventNumber       = evbId.eventNumber();
  block->resyncCount       = evbId.resyncCount();
  block->blockNb           = blockNb;
  
  frlHeader->trigno        = evbId.eventNumber();
  frlHeader->segno         = blockNb;
  
  frlHeader->segsize       = nbFedBytesWritten;
  
  if (isLastBlockOfSuperFragment)
  {
    frlHeader->segsize |= FRL_LAST_SEGM;
  }
}


void evb::SuperFragmentGenerator::setNbBlocksInSuperFragment
(
  toolbox::mem::Reference* bufRef,
  const uint32_t nbBlocks
) const
{
  while (bufRef != 0)
  {
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
      (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
    block->nbBlocksInSuperFragment = nbBlocks;
    
    bufRef = bufRef->getNextReference();
  }
}


void evb::SuperFragmentGenerator::fillTriggerPayload
(
  unsigned char* fedPtr,
  const uint32_t eventNumber,
  const L1Information& l1Info
) const
{
  using namespace evtn;
  
  //set offsets based on record scheme 
  evm_board_setformat(dummyFedPayloadSize_+sizeof(fedh_t)+sizeof(fedt_t));
  
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


void evb::SuperFragmentGenerator::updateCRC
(
  const unsigned char* fedPtr
) const
{
  const size_t fedSize = sizeof(fedh_t) + dummyFedPayloadSize_ + sizeof(fedt_t);
  fedt_t* fedTrailer = (fedt_t*)(fedPtr + fedSize - sizeof(fedt_t));
  
  // Force CRC field to zero before re-computing the CRC.
  // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
  fedTrailer->conscheck = 0;
  
  unsigned short crc = evf::compute_crc(fedPtr,fedSize);
  fedTrailer->conscheck = (crc << FED_CRCS_SHIFT);
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
