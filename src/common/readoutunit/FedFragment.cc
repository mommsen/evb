#include <sstream>

#include "evb/Constants.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "evb/readoutunit/FedFragment.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"


evb::CRCCalculator evb::readoutunit::FedFragment::crcCalculator_;

evb::readoutunit::FedFragment::FedFragment(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
  : fedSize_(0),isCorrupted_(false),bufRef_(bufRef),cache_(cache)
{
  assert(bufRef);
  const I2O_DATA_READY_MESSAGE_FRAME* msg = (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  fedId_ = msg->fedid;
  eventNumber_ = msg->triggerno;
  payload_ = ((unsigned char*)msg)+sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  length_ = msg->partLength;
  assert( length_ == msg->totalLength );
}


evb::readoutunit::FedFragment::FedFragment(uint16_t fedId, const EvBid& evbId, toolbox::mem::Reference* bufRef)
  : fedId_(fedId),evbId_(evbId),fedSize_(0),isCorrupted_(false),
    bufRef_(bufRef),cache_(0)
{
  eventNumber_ = evbId.eventNumber();
  payload_ = (unsigned char*)bufRef->getDataLocation();
  length_ = bufRef->getDataSize();
}


evb::readoutunit::FedFragment::FedFragment(uint16_t fedId, uint32_t eventNumber, unsigned char* payload, size_t length)
  : fedId_(fedId),eventNumber_(eventNumber),fedSize_(0),isCorrupted_(false),
    payload_(payload),length_(length),bufRef_(0),cache_(0)
{}


evb::readoutunit::FedFragment::~FedFragment()
{
  if ( bufRef_ )
  {
    // break any chain
    bufRef_->setNextReference(0);
    if ( cache_ )
      cache_->grantFrame(bufRef_);
    else
      bufRef_->release();
  }
  else
  {
    // Release payload
  }
}


unsigned char* evb::readoutunit::FedFragment::getFedPayload() const
{
  return payload_ + sizeof(ferolh_t);
}


uint32_t evb::readoutunit::FedFragment::getFedSize()
{
  if ( fedSize_ == 0 )
    calculateSize();
  return fedSize_;
}


void evb::readoutunit::FedFragment::calculateSize()
{
  fedSize_ = 0;
  uint32_t ferolOffset = 0;
  ferolh_t* ferolHeader;

  do
  {
    ferolHeader = (ferolh_t*)(payload_ + ferolOffset);
    assert( ferolHeader->signature() == FEROL_SIGNATURE );

    const uint32_t dataLength = ferolHeader->data_length();
    fedSize_ += dataLength;
    ferolOffset += dataLength + sizeof(ferolh_t);
  }
  while ( !ferolHeader->is_last_packet() && ferolOffset < length_ );
}


void evb::readoutunit::FedFragment::checkIntegrity(FedErrors& fedErrors, const uint32_t checkCRC)
{
  uint16_t crc = 0xffff;
  const bool computeCRC = ( checkCRC > 0 && eventNumber_ % checkCRC == 0 );
  fedSize_ = 0;
  uint32_t usedSize = 0;
  unsigned char* pos = payload_;
  ferolh_t* ferolHeader;

  do
  {
    ferolHeader = (ferolh_t*)pos;

    if ( ferolHeader->signature() != FEROL_SIGNATURE )
    {
      isCorrupted_ = true;
      ++fedErrors.corruptedEvents;
      std::ostringstream msg;
      msg << "Expected FEROL header signature " << std::hex << FEROL_SIGNATURE;
      msg << ", but found " << std::hex << ferolHeader->signature();
      msg << " in FED " << std::dec << fedId_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( fedId_ != ferolHeader->fed_id() )
    {
      isCorrupted_ = true;
      ++fedErrors.corruptedEvents;
      std::ostringstream msg;
      msg << "Mismatch of FED id in FEROL header:";
      msg << " expected " << fedId_ << ", but got " << ferolHeader->fed_id();
      msg << " for event " << eventNumber_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( eventNumber_ != ferolHeader->event_number() )
    {
      isCorrupted_ = true;
      ++fedErrors.corruptedEvents;
      std::ostringstream msg;
      msg << "Mismatch of event number in FEROL header:";
      msg << " expected " << eventNumber_ << ", but got " << ferolHeader->event_number();
      msg << " for FED " << fedId_;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    pos += sizeof(ferolh_t);

    if ( ferolHeader->is_first_packet() )
    {
      const fedh_t* fedHeader = (fedh_t*)pos;

      if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
      {
        isCorrupted_ = true;
        ++fedErrors.corruptedEvents;
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
        ++fedErrors.corruptedEvents;
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
        ++fedErrors.corruptedEvents;
        std::ostringstream oss;
        oss << "FED header \"sourceId\" " << sourceId << " does not match";
        oss << " the FED id " << fedId_ << " found in I2O_DATA_READY_MESSAGE_FRAME";
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }
    }

    const uint32_t dataLength = ferolHeader->data_length();

    if ( computeCRC )
    {
      if ( ferolHeader->is_last_packet() )
        crcCalculator_.compute(crc,pos,dataLength-sizeof(fedt_t)); // omit the FED trailer
      else
        crcCalculator_.compute(crc,pos,dataLength);
    }

    pos += dataLength;
    fedSize_ += dataLength;
    usedSize += dataLength + sizeof(ferolh_t);
  }
  while ( !ferolHeader->is_last_packet() );

  fedt_t* trailer = (fedt_t*)(pos - sizeof(fedt_t));

  if ( FED_TCTRLID_EXTRACT(trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    isCorrupted_ = true;
    ++fedErrors.corruptedEvents;
    std::ostringstream oss;
    oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    oss << " but got event size 0x" << trailer->eventsize;
    oss << " and conscheck 0x" << trailer->conscheck;
    oss << " for FED " << std::dec << fedId_;
    oss << " with expected size " << fedSize_ << " Bytes";
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  const uint32_t evsz = FED_EVSZ_EXTRACT(trailer->eventsize)<<3;
  if ( evsz != fedSize_ )
  {
    isCorrupted_ = true;
    ++fedErrors.corruptedEvents;
    std::ostringstream oss;
    oss << "Inconsistent event size for FED " << fedId_ << ":";
    oss << " FED trailer claims " << evsz << " Bytes,";
    oss << " while sum of FEROL headers yield " << fedSize_;
    if (trailer->conscheck & 0x8004)
    {
      oss << ". Trailer indicates that " << trailerBitToString(trailer->conscheck);
    }
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  if ( computeCRC )
  {
    // Force C,F,R & CRC field to zero before re-computing the CRC.
    // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
    const uint32_t conscheck = trailer->conscheck;
    trailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);
    crcCalculator_.compute(crc,pos-sizeof(fedt_t),sizeof(fedt_t));
    trailer->conscheck = conscheck;

    const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);
    if ( trailerCRC != crc )
    {
      if ( evb::isFibonacci( ++fedErrors.crcErrors ) )
      {
        std::ostringstream oss;
        oss << "Received " << fedErrors.crcErrors << " events with wrong CRC checksum from FED " << fedId_ << ":";
        oss << " FED trailer claims 0x" << std::hex << trailerCRC;
        oss << ", but recalculation gives 0x" << crc;
        XCEPT_RAISE(exception::CRCerror, oss.str());
      }
    }
  }

  if ( (trailer->conscheck & 0xC004) &&
       evb::isFibonacci( ++fedErrors.fedErrors ) )
  {
    std::ostringstream oss;
    oss << "Received " << fedErrors.fedErrors << " events from FED " << fedId_ << " where ";
    oss << trailerBitToString(trailer->conscheck);
    XCEPT_RAISE(exception::FEDerror, oss.str());
  }
}


std::string evb::readoutunit::FedFragment::trailerBitToString(const uint32_t conscheck) const
{
  if ( conscheck & 0x4 ) // FED CRC error (R bit)
  {
    return "wrong FED CRC checksum was found by the FEROL (FED trailer R bit is set)";
  }
  if ( conscheck & 0x4000 ) // wrong FED id (F bit)
  {
    return "the FED id not expected by the FEROL (FED trailer F bit is set)";
  }
  if ( conscheck & 0x8000 ) // slink CRC error (C bit)
  {
    return "wrong slink CRC checksum was found by the FEROL (FED trailer C bit is set)";
  }
  return "";
}

void evb::readoutunit::FedFragment::dump
(
  std::ostream& s,
  const std::string& reasonForDump
)
{
  s << "==================== DUMP ======================" << std::endl;
  s << "Reason for dump: " << reasonForDump << std::endl;

  s << "Buffer data location (hex): " << toolbox::toString("%x", getPayload()) << std::endl;
  s << "Buffer data size     (dec): " << getLength() << std::endl;
  s << "FED size             (dec): " << getFedSize() << std::endl;
  s << "FED id               (dec): " << getFedId() << std::endl;
  s << "Trigger no           (dec): " << getEventNumber() << std::endl;
  s << "EvB id                    : " << getEvBid() << std::endl;
  DumpUtility::dumpBlockData(s, payload_, length_);

  s << "================ END OF DUMP ===================" << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
