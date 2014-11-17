#include "i2o/Method.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/Constants.h"
#include "evb/Exception.h"
#include "evb/FragmentTracker.h"
#include "evb/dummyFEROL/FragmentGenerator.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"

#include <assert.h>
#include <math.h>
#include <sstream>
#include <sys/time.h>

evb::test::dummyFEROL::FragmentGenerator::FragmentGenerator() :
  frameSize_(0),
  fedSize_(0),
  usePlayback_(false)
{}


void evb::test::dummyFEROL::FragmentGenerator::configure
(
  const uint32_t fedId,
  const bool usePlayback,
  const std::string& playbackDataFile,
  const uint32_t frameSize,
  const uint32_t fedSize,
  const bool computeCRC,
  const bool useLogNormal,
  const uint32_t fedSizeStdDev,
  const uint32_t minFedSize,
  const uint32_t maxFedSize,
  const size_t fragmentPoolSize,
  const uint32_t fakeLumiSectionDuration,
  const uint32_t maxTriggerRate
)
{
  if (fedId > FED_COUNT)
  {
    std::ostringstream oss;
    oss << "The fedSourceId " << fedId;
    oss << " is larger than maximal value FED_COUNT=" << FED_COUNT;
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  fedId_ = fedId;

  fedSize_ = fedSize;
  if ( fedSize_ % 8 != 0 )
  {
    std::ostringstream oss;
    oss << "The requested FED payload of " << fedSize_ << " bytes";
    oss << " is not a multiple of 8 bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  frameSize_ = frameSize;
  if ( frameSize_ < FEROL_BLOCK_SIZE )
  {
    std::ostringstream oss;
    oss << "The frame size " << frameSize_ ;
    oss << " must at least hold one FEROL block of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  if ( frameSize_ % FEROL_BLOCK_SIZE != 0 )
  {
    std::ostringstream oss;
    oss << "The frame size " << frameSize_ ;
    oss << " must be a multiple of the FEROL block size of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  usePlayback_ = usePlayback;
  evbIdFactory_.setFakeLumiSectionDuration(fakeLumiSectionDuration);

  toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
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
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(fragmentPoolSize);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for dummy fragments", e);
  }

  fragmentTracker_.reset(
    new FragmentTracker(fedId,fedSize,useLogNormal,fedSizeStdDev,minFedSize,maxFedSize,computeCRC)
  );
  fragmentTracker_->setMaxTriggerRate(maxTriggerRate);

  playbackData_.clear();
  if ( usePlayback )
    cacheData(playbackDataFile);
}


void evb::test::dummyFEROL::FragmentGenerator::setMaxTriggerRate(const uint32_t rate)
{
  if ( fragmentTracker_.get() )
    fragmentTracker_->setMaxTriggerRate(rate);
}


void evb::test::dummyFEROL::FragmentGenerator::cacheData(const std::string& playbackDataFile)
{
  toolbox::mem::Reference* bufRef = 0;
  uint32_t dummy = 0;
  fillData(bufRef,dummy,dummy,dummy,dummy,dummy,dummy);
  playbackData_.push_back(bufRef);
  playbackDataPos_ = playbackData_.begin();
}


void evb::test::dummyFEROL::FragmentGenerator::reset()
{
  playbackDataPos_ = playbackData_.begin();
  evbIdFactory_.reset(0);
  fragmentTracker_->startRun();
  evbId_ = evbIdFactory_.getEvBid();
}


bool evb::test::dummyFEROL::FragmentGenerator::getData
(
  toolbox::mem::Reference*& bufRef,
  const uint32_t stopAtEventNumber,
  uint32_t& lastEventNumber,
  uint32_t& skipNbEvents,
  uint32_t& duplicateNbEvents,
  uint32_t& corruptNbEvents,
  uint32_t& nbCRCerrors
)
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
    return fillData(bufRef,stopAtEventNumber,lastEventNumber,skipNbEvents,duplicateNbEvents,corruptNbEvents,nbCRCerrors);
  }
}


bool evb::test::dummyFEROL::FragmentGenerator::fillData
(
  toolbox::mem::Reference*& bufRef,
  const uint32_t stopAtEventNumber,
  uint32_t& lastEventNumber,
  uint32_t& skipNbEvents,
  uint32_t& duplicateNbEvents,
  uint32_t& corruptNbEvents,
  uint32_t& nbCRCerrors
)
{
  for ( uint32_t i = 0; i < skipNbEvents; ++i )
  {
    evbId_ = evbIdFactory_.getEvBid();
    skipNbEvents = 0;
  }

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
  memset(bufRef->getDataLocation(), 0, bufRef->getBuffer()->getSize());

  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);
  uint32_t usedFrameSize = 0;
  uint32_t remainingFedSize = fragmentTracker_->startFragment(evbId_);
  // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  uint16_t ferolBlocks = (remainingFedSize + ferolPayloadSize - 1)/ferolPayloadSize;

  while ( (usedFrameSize + remainingFedSize + ferolBlocks*sizeof(ferolh_t)) <= frameSize_ &&
          (stopAtEventNumber == 0 || lastEventNumber < stopAtEventNumber) )
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
      frame += sizeof(ferolh_t);

      const size_t filledBytes = fragmentTracker_->fillData(frame, length);
      //const size_t filledBytes = length;

      if ( nbCRCerrors > 0 && packetNumber == 0 )
      {
        unsigned char* payload = frame +
          ( filledBytes / 2 );
        *payload ^= 0x1;
        --nbCRCerrors;
      }

      ferolHeader->set_data_length(filledBytes);
      ferolHeader->set_fed_id(fedId_);
      ferolHeader->set_event_number( evbId_.eventNumber() );

      // assert( ferolHeader->signature() == FEROL_SIGNATURE );
      // assert( ferolHeader->packet_number() == packetNumber );
      // assert( ferolHeader->data_length() == filledBytes );
      // assert( ferolHeader->data_length() == length );
      // assert( ferolHeader->fed_id() == fedId_ );
      // assert( ferolHeader->event_number() == evbId_.eventNumber() );

      frame += filledBytes;
      usedFrameSize += filledBytes + sizeof(ferolh_t);

      ++packetNumber;
      assert(packetNumber < 2048);
    }
    assert( packetNumber == ferolBlocks );

    if ( corruptNbEvents > 0 )
    {
      fedt_t* fedTrailer = (fedt_t*)(frame - sizeof(fedt_t));
      fedTrailer->eventsize ^= 0x10;
      --corruptNbEvents;
    }

    if ( duplicateNbEvents > 0 )
      --duplicateNbEvents;
    else
    {
      lastEventNumber = evbId_.eventNumber();
      evbId_ = evbIdFactory_.getEvBid();
    }

    remainingFedSize = fragmentTracker_->startFragment(evbId_);
    ferolBlocks = (remainingFedSize + ferolPayloadSize - 1)/ferolPayloadSize;
  }

  assert( usedFrameSize <= frameSize_ );
  bufRef->setDataSize(usedFrameSize);


  return true;
}


toolbox::mem::Reference* evb::test::dummyFEROL::FragmentGenerator::clone
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


void evb::test::dummyFEROL::FragmentGenerator::fillTriggerPayload
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


void evb::test::dummyFEROL::FragmentGenerator::updateCRC
(
  const unsigned char* fedPtr
) const
{
  // fedt_t* fedTrailer = (fedt_t*)(fedPtr + fedSize_ - sizeof(fedt_t));

  // // Force C,F,R & CRC field to zero before re-computing the CRC.
  // // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
  // fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);

  // unsigned short crc = compute_crc(fedPtr,fedSize_);
  // fedTrailer->conscheck = (crc << FED_CRCS_SHIFT);
}


evb::test::dummyFEROL::L1Information::L1Information()
{
  reset();
}


void evb::test::dummyFEROL::L1Information::reset()
{
  strncpy(reason, "none", sizeof(reason));
  isValid           = false;
  runNumber         = 0UL;
  lsNumber          = 0UL;
  bunchCrossing     = 0UL;
  orbitNumber       = 0UL;
  eventType         = 0U;
  l1Technical       = 0ULL;
  l1Decision_0_63   = 0ULL;
  l1Decision_64_127 = 0ULL;
}


const evb::test::dummyFEROL::L1Information& evb::test::dummyFEROL::L1Information::operator=
(
  const L1Information& l1Info
)
{
  strncpy(reason, l1Info.reason, sizeof(reason));
  isValid           = l1Info.isValid;
  runNumber         = l1Info.runNumber;
  lsNumber          = l1Info.lsNumber;
  bunchCrossing     = l1Info.bunchCrossing;
  orbitNumber       = l1Info.orbitNumber;
  eventType         = l1Info.eventType;
  l1Technical       = l1Info.l1Technical;
  l1Decision_0_63   = l1Info.l1Decision_0_63;
  l1Decision_64_127 = l1Info.l1Decision_64_127;
  return *this;
}




/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
