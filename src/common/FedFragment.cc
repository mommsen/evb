#include <sstream>

#include "evb/Exception.h"
#include "evb/FedFragment.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"


evb::CRCCalculator evb::FedFragment::crcCalculator_;


evb::FedFragment::FedFragment(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
  : bufRef_(bufRef),cache_(cache)
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


uint32_t evb::FedFragment::checkIntegrity(toolbox::mem::Reference* bufRef, const bool checkCRC)
{
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  uint32_t payloadSize = frame->partLength;
  const uint16_t fedId = frame->fedid;
  const uint32_t eventNumber = frame->triggerno;

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
      std::ostringstream msg;
      msg << "Expected FEROL header signature " << std::hex << FEROL_SIGNATURE;
      msg << ", but found " << std::hex << ferolHeader->signature();
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( eventNumber != ferolHeader->event_number() )
    {
      std::ostringstream msg;
      msg << "Mismatch of event number in FEROL header:";
      msg << " expected " << eventNumber << ", but got " << ferolHeader->event_number();
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    if ( fedId != ferolHeader->fed_id() )
    {
      std::ostringstream msg;
      msg << "Mismatch of FED id in FEROL header:";
      msg << " expected " << fedId << ", but got " << ferolHeader->fed_id();
      msg << " for event " << eventNumber;
      XCEPT_RAISE(exception::DataCorruption, msg.str());
    }

    payload += sizeof(ferolh_t);

    if ( ferolHeader->is_first_packet() )
    {
      const fedh_t* fedHeader = (fedh_t*)payload;

      if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
      {
        std::ostringstream oss;
        oss << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
        oss << " but got event id 0x" << std::hex << fedHeader->eventid;
        oss << " and source id 0x" << std::hex << fedHeader->sourceid;
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }

      const uint32_t eventId = FED_LVL1_EXTRACT(fedHeader->eventid);
      if ( eventId != eventNumber)
      {
        std::ostringstream oss;
        oss << "FED header \"eventid\" " << eventId << " does not match";
        oss << " the eventNumber " << eventNumber << " found in I2O_DATA_READY_MESSAGE_FRAME";
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }

      const uint32_t sourceId = FED_SOID_EXTRACT(fedHeader->sourceid);
      if ( sourceId != fedId )
      {
        std::ostringstream oss;
        oss << "FED header \"sourceId\" " << sourceId << " does not match";
        oss << " the FED id " << fedId << " found in I2O_DATA_READY_MESSAGE_FRAME";
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
      bufRef = bufRef->getNextReference();

      if ( ! bufRef )
      {
        std::ostringstream msg;
        msg << "Premature end of FEROL data:";
        msg << " expected " << usedSize << " more Bytes,";
        msg << " but toolbox::mem::Reference chain has ended";
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      payload = (unsigned char*)bufRef->getDataLocation();
      frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
      payloadSize = frame->partLength;
      payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

      if ( fedId != frame->fedid )
      {
        std::ostringstream msg;
        msg << "Inconsistent FED id:";
        msg << " first I2O_DATA_READY_MESSAGE_FRAME was from FED id " << fedId;
        msg << " while the current has FED id " << frame->fedid;
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      if ( eventNumber != frame->triggerno )
      {
        std::ostringstream msg;
        msg << "Inconsistent event number:";
        msg << " first I2O_DATA_READY_MESSAGE_FRAME was from event " << eventNumber;
        msg << " while the current is from event " << frame->triggerno;
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }
    }
  }
  while ( !ferolHeader->is_last_packet() );

  fedt_t* trailer = (fedt_t*)(payload - sizeof(fedt_t));

  if ( FED_TCTRLID_EXTRACT(trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    std::ostringstream oss;
    oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    oss << " but got event size 0x" << std::hex << trailer->eventsize;
    oss << " and conscheck 0x" << std::hex << trailer->conscheck;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  const uint32_t evsz = FED_EVSZ_EXTRACT(trailer->eventsize)<<3;
  if ( evsz != fedSize )
  {
    std::ostringstream oss;
    oss << "Inconsistent event size:";
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
      std::ostringstream oss;
      oss << "Wrong CRC checksum:" << std::hex;
      oss << " FED trailer claims 0x" << trailerCRC;
      oss << ", but recalculation gives 0x" << crc;
      XCEPT_RAISE(exception::CRCerror, oss.str());
    }
  }

  return fedSize;
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
