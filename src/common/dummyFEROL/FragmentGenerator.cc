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
#include <fstream>
#include <math.h>
#include <sstream>

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
    std::ostringstream msg;
    msg << "The FED " << fedId;
    msg << " is larger than maximal value FED_COUNT=" << FED_COUNT;
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  fedId_ = fedId;

  fedSize_ = fedSize;
  if ( fedSize_ % 8 != 0 )
  {
    std::ostringstream msg;
    msg << "The requested FED payload of " << fedSize_ << " bytes";
    msg << " is not a multiple of 8 bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  frameSize_ = frameSize;
  if ( frameSize_ < FEROL_BLOCK_SIZE )
  {
    std::ostringstream msg;
    msg << "The frame size " << frameSize_ ;
    msg << " must at least hold one FEROL block of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  if ( frameSize_ % FEROL_BLOCK_SIZE != 0 )
  {
    std::ostringstream msg;
    msg << "The frame size " << frameSize_ ;
    msg << " must be a multiple of the FEROL block size of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
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
    new FragmentTracker(fedId,fedSize,fedSizeStdDev,minFedSize,maxFedSize,computeCRC)
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
  std::ifstream is(playbackDataFile.c_str(), std::ifstream::binary);

  if (is) {
    // get length of file:
    is.seekg(0, is.end);
    int fileLength = is.tellg();
    is.seekg(0, is.beg);

    char* buffer = new char[fileLength];
    is.read(buffer,fileLength);

    assert( is.gcount() == fileLength );

    is.close();

    while ( fileLength > 0 )
    {
      fedt_t* fedTrailer =  (fedt_t*)(&buffer[fileLength-sizeof(fedt_t)]);
      assert( FED_TCTRLID_EXTRACT(fedTrailer->eventsize) == FED_SLINK_END_MARKER );

      uint32_t remainingFedSize = FED_EVSZ_EXTRACT(fedTrailer->eventsize)<<3;
      fileLength -= remainingFedSize;

      fedh_t* fedHeader = (fedh_t*)(&buffer[fileLength]);
      assert( FED_HCTRLID_EXTRACT(fedHeader->eventid) == FED_SLINK_START_MARKER );

      // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
      const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);
      const uint16_t ferolBlocks = (remainingFedSize + ferolPayloadSize - 1)/ferolPayloadSize;
      const uint32_t fragmentSize = remainingFedSize + ferolBlocks*sizeof(ferolh_t);

      toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->getFrame(fragmentPool_,fragmentSize);
      bufRef->setDataSize(fragmentSize);
      unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
      memset(payload,0,fragmentSize);

      for (uint16_t packetNumber = 0; packetNumber < ferolBlocks; ++packetNumber)
      {
        assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
        uint32_t length;

        ferolh_t* ferolHeader = (ferolh_t*)payload;
        ferolHeader->set_signature();
        ferolHeader->set_packet_number(packetNumber);
        ferolHeader->set_fed_id( FED_SOID_EXTRACT(fedHeader->sourceid) );
        ferolHeader->set_event_number( FED_LVL1_EXTRACT(fedHeader->eventid) );

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

        ferolHeader->set_data_length(length);

        remainingFedSize -= length;
        payload += sizeof(ferolh_t);

        memcpy(payload,&buffer[fileLength+packetNumber*ferolPayloadSize],length);

        payload += length;
      }

      playbackData_.push_back(bufRef);

    }

    delete[] buffer;
  }

  playbackDataPos_ = playbackData_.rbegin();

  //std::cout << "**** Cached " << playbackData_.size() << " events" << std::endl;

}


void evb::test::dummyFEROL::FragmentGenerator::reset()
{
  playbackDataPos_ = playbackData_.rbegin();
  evbIdFactory_.reset(0);
  fragmentTracker_->startRun();
  evbId_ = evbIdFactory_.getEvBid();
}


void evb::test::dummyFEROL::FragmentGenerator::resyncAtEvent(const uint32_t eventNumber)
{
  evbIdFactory_.resyncAtEvent(eventNumber);
}


bool evb::test::dummyFEROL::FragmentGenerator::getData
(
  toolbox::mem::Reference*& bufRef,
  const uint32_t stopAtEventNumber,
  uint32_t& lastEventNumber,
  uint32_t& skipNbEvents,
  uint32_t& duplicateNbEvents,
  uint32_t& corruptNbEvents,
  uint32_t& nbCRCerrors,
  uint32_t& nbBXerrors
)
{
  if ( usePlayback_ )
  {
    if ( (*playbackDataPos_)->getBuffer()->getRefCounter() > 1024 ) return false;
    bufRef = (*playbackDataPos_)->duplicate();

    if ( ++playbackDataPos_ == playbackData_.rend() )
      playbackDataPos_ = playbackData_.rbegin();
    return true;
  }
  else
  {
    return fillData(bufRef,stopAtEventNumber,lastEventNumber,skipNbEvents,duplicateNbEvents,corruptNbEvents,nbCRCerrors,nbBXerrors);
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
  uint32_t& nbCRCerrors,
  uint32_t& nbBXerrors
)
{
  for ( uint32_t i = 0; i < skipNbEvents; ++i )
  {
    evbId_ = evbIdFactory_.getEvBid();
  }
  skipNbEvents = 0;

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

      if (packetNumber == 0)
      {
        if ( nbCRCerrors > 0 )
        {
          unsigned char* payload = frame +
            ( filledBytes / 2 );
          *payload ^= 0x1;
          --nbCRCerrors;
        }
        if ( nbBXerrors > 0 )
        {
          fedh_t* fedHeader = (fedh_t*)(frame);
          fedHeader->sourceid ^= (0x10 << FED_BXID_SHIFT);
          --nbBXerrors;
        }
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


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
