#include <sstream>

#include "evb/Exception.h"
#include "evb/FedFragment.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"


evb::FedFragment::FedFragment(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
  : isCorrupted_(false),hasCRCerror_(false),bufRef_(bufRef),cache_(cache)
{
  assert(bufRef);
  const I2O_DATA_READY_MESSAGE_FRAME* msg = (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  fedId_ = msg->fedid;
  eventNumber_ = msg->triggerno;
}


evb::FedFragment::~FedFragment()
{
  // break any chain
  bufRef_->setNextReference(0);
  if ( cache_ )
    cache_->grantFrame(bufRef_);
  else
    bufRef_->release();
}


unsigned char* evb::FedFragment::getFedPayload() const
{
  return (unsigned char*)(bufRef_->getDataLocation())
    + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + sizeof(ferolh_t);
}


uint32_t evb::FedFragment::checkIntegrity(const bool checkCRC)
{
  toolbox::mem::Reference* currentBufRef = bufRef_;
  unsigned char* payload = (unsigned char*)currentBufRef->getDataLocation();
  I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  uint32_t payloadSize = frame->partLength;

  payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

  uint16_t crc = 0xffff;
  uint32_t fedSize = 0;
  uint32_t usedSize = 0;
  ferolh_t* ferolHeader;

  do
  {
    ferolHeader = (ferolh_t*)payload;

    if ( ferolHeader->signature() != FEROL_SIGNATURE )
    {
      isCorrupted_ = true;
      std::ostringstream msg;
      msg << "Expected FEROL header signature " << std::hex << FEROL_SIGNATURE;
      msg << ", but found " << std::hex << ferolHeader->signature();
      msg << " in FED " << std::dec << fedId_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( fedId_ != ferolHeader->fed_id() )
    {
      isCorrupted_ = true;
      std::ostringstream msg;
      msg << "Mismatch of FED id in FEROL header:";
      msg << " expected " << fedId_ << ", but got " << ferolHeader->fed_id();
      msg << " for event " << eventNumber_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( eventNumber_ != ferolHeader->event_number() )
    {
      isCorrupted_ = true;
      std::ostringstream msg;
      msg << "Mismatch of event number in FEROL header:";
      msg << " expected " << eventNumber_ << ", but got " << ferolHeader->event_number();
      msg << " for FED " << fedId_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    payload += sizeof(ferolh_t);

    if ( ferolHeader->is_first_packet() )
    {
      const fedh_t* fedHeader = (fedh_t*)payload;

      if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
      {
        isCorrupted_ = true;
        std::ostringstream oss;
        oss << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
        oss << " but got event id 0x" << fedHeader->eventid;
        oss << " and source id 0x" << fedHeader->sourceid;
        oss << " for FED " << std::dec << fedId_;
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }

      const uint32_t eventId = FED_LVL1_EXTRACT(fedHeader->eventid);
      if ( eventId != eventNumber_ )
      {
        isCorrupted_ = true;
        std::ostringstream oss;
        oss << "FED header \"eventid\" " << eventId << " does not match";
        oss << " the eventNumber " << eventNumber_ << " found in I2O_DATA_READY_MESSAGE_FRAME";
        oss << " of FED " << fedId_;
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }

      const uint32_t sourceId = FED_SOID_EXTRACT(fedHeader->sourceid);
      if ( sourceId != fedId_ )
      {
        isCorrupted_ = true;
        std::ostringstream oss;
        oss << "FED header \"sourceId\" " << sourceId << " does not match";
        oss << " the FED id " << fedId_ << " found in I2O_DATA_READY_MESSAGE_FRAME";
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }
    }

    const uint32_t dataLength = ferolHeader->data_length();

    if ( checkCRC )
    {
      if ( ferolHeader->is_last_packet() )
        crcCalculator_.compute(crc,payload,dataLength-sizeof(fedt_t)); // omit the FED trailer
      else
        crcCalculator_.compute(crc,payload,dataLength);
    }

    payload += dataLength;
    fedSize += dataLength;
    usedSize += dataLength + sizeof(ferolh_t);

    if ( usedSize >= payloadSize && !ferolHeader->is_last_packet() )
    {
      usedSize -= payloadSize;
      currentBufRef = currentBufRef->getNextReference();

      if ( ! currentBufRef )
      {
        isCorrupted_ = true;
        std::ostringstream msg;
        msg << "Premature end of FEROL data for FED " << fedId_ << ":";
        msg << " expected " << usedSize << " more Bytes,";
        msg << " but toolbox::mem::Reference chain has ended";
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      payload = (unsigned char*)currentBufRef->getDataLocation();
      frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
      payloadSize = frame->partLength;
      payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

      if ( fedId_ != frame->fedid )
      {
        isCorrupted_ = true;
        std::ostringstream msg;
        msg << "Inconsistent FED id:";
        msg << " first I2O_DATA_READY_MESSAGE_FRAME was from FED id " << fedId_;
        msg << " while the current has FED id " << frame->fedid;
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      if ( eventNumber_ != frame->triggerno )
      {
        isCorrupted_ = true;
        std::ostringstream msg;
        msg << "Inconsistent event number for FED " << fedId_ << ":";
        msg << " first I2O_DATA_READY_MESSAGE_FRAME was from event " << eventNumber_;
        msg << " while the current is from event " << frame->triggerno;
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }
    }
  }
  while ( !ferolHeader->is_last_packet() );

  fedt_t* trailer = (fedt_t*)(payload - sizeof(fedt_t));

  if ( FED_TCTRLID_EXTRACT(trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    isCorrupted_ = true;
    std::ostringstream oss;
    oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    oss << " but got event size 0x" << trailer->eventsize;
    oss << " and conscheck 0x" << trailer->conscheck;
    oss << " for FED " << std::dec << fedId_;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  const uint32_t evsz = FED_EVSZ_EXTRACT(trailer->eventsize)<<3;
  if ( evsz != fedSize )
  {
    isCorrupted_ = true;
    std::ostringstream oss;
    oss << "Inconsistent event size for FED " << fedId_ << ":";
    oss << " FED trailer claims " << evsz << " Bytes,";
    oss << " while sum of FEROL headers yield " << fedSize;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  if ( checkCRC )
  {
    // Force CRC & R field to zero before re-computing the CRC.
    // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
    const uint32_t conscheck = trailer->conscheck;
    trailer->conscheck &= ~(FED_CRCS_MASK | 0x4);
    crcCalculator_.compute(crc,payload-sizeof(fedt_t),sizeof(fedt_t));
    trailer->conscheck = conscheck;

    const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);
    if ( trailerCRC != crc )
    {
      hasCRCerror_ = true;
      std::ostringstream oss;
      oss << "Wrong CRC checksum for FED " << fedId_ << ":" << std::hex;
      oss << " FED trailer claims 0x" << trailerCRC;
      oss << ", but recalculation gives 0x" << crc;
      XCEPT_RAISE(exception::CRCerror, oss.str());
    }
  }

  if ( trailer->conscheck & 0x4 ) // FED CRC error (R bit)
  {
    std::ostringstream oss;
    oss << "Wrong FED CRC checksum for FED " << fedId_ << " found in FED trailer (R bit)";
    XCEPT_RAISE(exception::FEDerror, oss.str());
  }

  if ( trailer->conscheck & 0x8000 ) // slink CRC error (C bit)
  {
    std::ostringstream oss;
    oss << "Wrong slink CRC checksum for FED " << fedId_ << " found in FED trailer (C bit)";
    XCEPT_RAISE(exception::FEDerror, oss.str());
  }

  return fedSize;
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
