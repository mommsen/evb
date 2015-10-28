#include "evb/Constants.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "evb/readoutunit/FedFragment.h"
#include "interface/shared/i2ogevb2g.h"


evb::CRCCalculator evb::readoutunit::FedFragment::crcCalculator_;

evb::readoutunit::FedFragment::FedFragment
(
  const uint16_t fedId,
  const bool isMasterFed,
  const EvBidFactoryPtr& evbIdFactory,
  const uint32_t checkCRC,
  uint32_t& fedErrorCount,
  uint32_t& crcErrors
)
  : typeOfNextComponent_(FEROL_HEADER),
    evbIdFactory_(evbIdFactory),checkCRC_(checkCRC),
    fedErrorCount_(fedErrorCount),crcErrors_(crcErrors),
    fedId_(fedId),isMasterFed_(isMasterFed),
    bxId_(FED_BXID_WIDTH+1),
    eventNumber_(0),fedSize_(0),
    isCorrupted_(false),isOutOfSequence_(false),
    hasCRCerror_(false),hasFEDerror_(false),isComplete_(false),
    bufRef_(0),cache_(0),tmpBufferSize_(0)
{}


evb::readoutunit::FedFragment::~FedFragment()
{
  toolbox::mem::Reference* nextBufRef;
  while ( bufRef_ )
  {
    nextBufRef = bufRef_->getNextReference();
    bufRef_->setNextReference(0);

    if ( cache_ )
      cache_->grantFrame(bufRef_);
    else if ( socketBuffers_.empty() )
      bufRef_->release();

    bufRef_ = nextBufRef;
  };
}


bool evb::readoutunit::FedFragment::append(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
{
  bufRef_ = bufRef;
  cache_ = cache;
  const I2O_DATA_READY_MESSAGE_FRAME* msg = (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  assert( msg->partLength == msg->totalLength );
  uint32_t usedSize = sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  return parse(bufRef,usedSize);
}


bool evb::readoutunit::FedFragment::append(const EvBid& evbId, toolbox::mem::Reference* bufRef)
{
  evbId_ = evbId;
  bufRef_ = bufRef;
  uint32_t usedSize = 0;
  return parse(bufRef,usedSize);
}


bool evb::readoutunit::FedFragment::append(SocketBufferPtr& socketBuffer, uint32_t& usedSize)
{
  socketBuffers_.push_back(socketBuffer);
  return parse(socketBuffer->getBufRef(), usedSize);
}


bool evb::readoutunit::FedFragment::parse(toolbox::mem::Reference* bufRef, uint32_t& usedSize)
{
  assert( ! isComplete_ );

  uint32_t remainingBufferSize = bufRef->getDataSize() - usedSize;
  const unsigned char* pos = (unsigned char*)bufRef->getDataLocation();

  iovec dataLocation;
  dataLocation.iov_base = (void*)(pos + usedSize);
  dataLocation.iov_len = 0;

  do
  {
    switch (typeOfNextComponent_)
    {
      case FEROL_HEADER:
      {
        ferolh_t* ferolHeader;
        if ( tmpBufferSize_ > 0 )
        {
          // previous buffer contained part of this header
          const uint32_t missingHeaderBytes = sizeof(ferolh_t)-tmpBufferSize_;
          memcpy(&tmpBuffer_[tmpBufferSize_],pos+usedSize,missingHeaderBytes);
          usedSize += missingHeaderBytes;
          remainingBufferSize -= missingHeaderBytes;
          tmpBufferSize_ = 0;
          ferolHeader = (ferolh_t*)&tmpBuffer_[0];
        }
        else if ( remainingBufferSize < sizeof(ferolh_t) )
        {
          // the header is not fully contained in this buffer
          // safe it for the next round
          if ( tmpBuffer_.size() < sizeof(ferolh_t) )
            tmpBuffer_.resize( sizeof(ferolh_t) );

          memcpy(&tmpBuffer_[0],pos+usedSize,remainingBufferSize);
          tmpBufferSize_ = remainingBufferSize;
          usedSize += remainingBufferSize;
          remainingBufferSize = 0;
          break;
        }
        else
        {
          ferolHeader = (ferolh_t*)(pos + usedSize);
          usedSize += sizeof(ferolh_t);
          remainingBufferSize -= sizeof(ferolh_t);
        }

        checkFerolHeader(ferolHeader);
        const uint32_t dataLength = ferolHeader->data_length();
        fedSize_ += dataLength;
        payloadLength_ = dataLength;

        if ( ferolHeader->is_first_packet() )
        {
          payloadLength_ -= sizeof(fedh_t);
          typeOfNextComponent_ = FED_HEADER;
        }
        else
        {
          typeOfNextComponent_ = FED_PAYLOAD;
        }

        if ( ferolHeader->is_last_packet() )
        {
          payloadLength_ -= sizeof(fedt_t);
          isLastFerolHeader_ = true;
        }
        else
        {
          isLastFerolHeader_ = false;
        }

        // skip the FEROL header
        if ( dataLocation.iov_len > 0 )
        {
          dataLocations_.push_back(dataLocation);
          dataLocation.iov_base = (void*)(pos + usedSize);
          dataLocation.iov_len = 0;
        }
        else
        {
          dataLocation.iov_base = (void*)(pos + usedSize);
        }

        break;
      }

      case FED_HEADER:
      {
        const fedh_t* fedHeader;
        if ( tmpBufferSize_ > 0 )
        {
          // previous buffer contained part of this header
          const uint32_t missingHeaderBytes = sizeof(fedh_t)-tmpBufferSize_;
          memcpy(&tmpBuffer_[tmpBufferSize_],pos+usedSize,missingHeaderBytes);
          usedSize += missingHeaderBytes;
          dataLocation.iov_len += missingHeaderBytes;
          remainingBufferSize -= missingHeaderBytes;
          tmpBufferSize_ = 0;
          fedHeader = (fedh_t*)&tmpBuffer_[0];
        }
        else if ( remainingBufferSize < sizeof(fedh_t) )
        {
          // the header is not fully contained in this buffer
          // safe it for the next round
          if ( tmpBuffer_.size() < sizeof(fedh_t) )
            tmpBuffer_.resize( sizeof(fedh_t) );

          memcpy(&tmpBuffer_[0],pos+usedSize,remainingBufferSize);
          tmpBufferSize_ = remainingBufferSize;
          usedSize += remainingBufferSize;
          dataLocation.iov_len += remainingBufferSize;
          remainingBufferSize = 0;
          break;
        }
        else
        {
          fedHeader = (fedh_t*)(pos + usedSize);
          usedSize += sizeof(fedh_t);
          dataLocation.iov_len += sizeof(fedh_t);
          remainingBufferSize -= sizeof(fedh_t);
        }

        checkFedHeader(fedHeader);
        if ( bxId_ > FED_BXID_WIDTH )
          bxId_ = FED_BXID_EXTRACT(fedHeader->sourceid);
        typeOfNextComponent_ = FED_PAYLOAD;
        break;
      }

      case FED_PAYLOAD:
      {
        if ( remainingBufferSize > payloadLength_ )
        {
          // the whole payload is in this buffer
          if ( isLastFerolHeader_ )
            typeOfNextComponent_ = FED_TRAILER;
          else
            typeOfNextComponent_ = FEROL_HEADER;
          remainingBufferSize -= payloadLength_;
          usedSize += payloadLength_;
          dataLocation.iov_len += payloadLength_;
        }
        else
        {
          // only part of the data is in this buffer
          payloadLength_ -= remainingBufferSize;
          usedSize += remainingBufferSize;
          dataLocation.iov_len += remainingBufferSize;
          remainingBufferSize = 0;
        }
        break;
      }

      case FED_TRAILER:
      {
        fedt_t* fedTrailer;
        if ( tmpBufferSize_ > 0 )
        {
          // previous buffer contained part of this header
          const uint32_t missingHeaderBytes = sizeof(fedt_t)-tmpBufferSize_;
          memcpy(&tmpBuffer_[tmpBufferSize_],pos+usedSize,missingHeaderBytes);
          usedSize += missingHeaderBytes;
          dataLocation.iov_len += missingHeaderBytes;
          remainingBufferSize -= missingHeaderBytes;
          tmpBufferSize_ = 0;
          fedTrailer = (fedt_t*)&tmpBuffer_[0];
        }
        else if ( remainingBufferSize < sizeof(fedt_t) )
        {
          // the header is not fully contained in this buffer
          // safe it for the next round
          if ( tmpBuffer_.size() < sizeof(fedt_t) )
            tmpBuffer_.resize( sizeof(fedt_t) );

          memcpy(&tmpBuffer_[0],pos+usedSize,remainingBufferSize);
          tmpBufferSize_ = remainingBufferSize;
          usedSize += remainingBufferSize;
          dataLocation.iov_len += remainingBufferSize;
          remainingBufferSize = 0;
          break;
        }
        else
        {
          fedTrailer = (fedt_t*)(pos + usedSize);
          usedSize += sizeof(fedt_t);
          dataLocation.iov_len += sizeof(fedt_t);
          remainingBufferSize -= sizeof(fedt_t);
        }

        isComplete_ = true;
        dataLocations_.push_back(dataLocation);

        checkFedTrailer(fedTrailer);

        try
        {
          if ( !evbId_.isValid() )
            evbId_ = evbIdFactory_->getEvBid(eventNumber_, bxId_, dataLocations_);
        }
        catch(exception::EventOutOfSequence& e)
        {
          isOutOfSequence_ = true;
          if ( isCorrupted_ || hasCRCerror_ || hasFEDerror_ )
            errorMsg_ << ". ";
          errorMsg_ << e.message();
        }

        reportErrors();

        return true;
      }
    }
  }
  while ( remainingBufferSize > 0 );

  if ( dataLocation.iov_len > 0 )
    dataLocations_.push_back(dataLocation);

  return false;
}


void evb::readoutunit::FedFragment::checkFerolHeader(const ferolh_t* ferolHeader)
{

  if ( ferolHeader->signature() != FEROL_SIGNATURE )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    msg << "Expected FEROL header signature " << std::hex << FEROL_SIGNATURE;
    msg << ", but found " << std::hex << ferolHeader->signature();
    msg << " in FED " << std::dec << fedId_;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( ferolHeader->is_first_packet() )
  {
    eventNumber_ = ferolHeader->event_number();
  }
  else if ( eventNumber_ != ferolHeader->event_number() )
  {
    isCorrupted_ = true;
    errorMsg_ << "mismatch of event number in FEROL header:";
    errorMsg_ << " expected " << eventNumber_ << ", but got " << ferolHeader->event_number();
  }

  if ( fedId_ != ferolHeader->fed_id() )
  {
    if ( isCorrupted_ )
      errorMsg_ << ", and ";
    else
      isCorrupted_ = true;
    errorMsg_ << "mismatch of FED id in FEROL header:";
    errorMsg_ << " expected FED " << fedId_ << ", but got " << ferolHeader->fed_id();
  }
}


void evb::readoutunit::FedFragment::checkFedHeader(const fedh_t* fedHeader)
{
  if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    msg << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
    msg << " but got event id 0x" << fedHeader->eventid;
    msg << " and source id 0x" << fedHeader->sourceid;
    msg << " for event " << std::dec << eventNumber_;
    msg << " from FED " << fedId_;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  const uint32_t eventId = FED_LVL1_EXTRACT(fedHeader->eventid);
  if ( eventId != eventNumber_ )
  {
    if ( isCorrupted_ )
      errorMsg_ << ", and ";
    else
      isCorrupted_ = true;
    errorMsg_ << "FED header \"eventid\" " << eventId;
    errorMsg_ << " does not match the eventNumber found in FEROL header";
  }

  const uint32_t sourceId = FED_SOID_EXTRACT(fedHeader->sourceid);
  if ( sourceId != fedId_ )
  {
    if ( isCorrupted_ )
      errorMsg_ << ", and ";
    else
      isCorrupted_ = true;
    errorMsg_ << "FED header \"sourceId\" " << sourceId;
    errorMsg_ << " does not match the FED " << fedId_ << " found in FEROL header";
  }
}


void evb::readoutunit::FedFragment::checkFedTrailer(fedt_t* fedTrailer)
{
  if ( FED_TCTRLID_EXTRACT(fedTrailer->eventsize) != FED_SLINK_END_MARKER )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    msg << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    msg << " but got event size 0x" << fedTrailer->eventsize;
    msg << " and conscheck 0x" << fedTrailer->conscheck;
    msg << " for event " << std::dec << eventNumber_;
    msg << " from FED " << fedId_;
    msg << " with expected size " << fedSize_ << " Bytes";
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  const uint32_t evsz = FED_EVSZ_EXTRACT(fedTrailer->eventsize)<<3;
  if ( evsz != fedSize_ )
  {
    if ( isCorrupted_ )
      errorMsg_ << ", and ";
    else
      isCorrupted_ = true;
    errorMsg_ << "inconsistent event size:";
    errorMsg_ << " FED trailer claims " << evsz << " Bytes,";
    errorMsg_ << " while sum of FEROL headers yield " << fedSize_;
  }

  checkCRC(fedTrailer);
  checkTrailerBits(fedTrailer->conscheck);
}


void evb::readoutunit::FedFragment::checkCRC(fedt_t* fedTrailer)
{
  if ( checkCRC_ == 0 || eventNumber_ % checkCRC_ != 0 ) return;

  // Force C,F,R & CRC field to zero before re-computing the CRC.
  // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
  const uint32_t conscheck = fedTrailer->conscheck;
  fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);

  // We need to assure here that the buffer given to the CRC calculator is Byte aligned.
  uint16_t crc = 0xffff;
  std::vector<unsigned char> buffer; //don't use tmpBuffer here as it might contain the FED trailer
  uint8_t overflow = 0;
  for ( DataLocations::const_iterator it = dataLocations_.begin(), itEnd = dataLocations_.end();
        it != itEnd; ++it)
  {
    uint8_t offset = 0;
    if ( overflow > 0 )
    {
      offset = 8 - overflow;
      memcpy(&buffer[overflow],(uint8_t*)it->iov_base,offset);
      crcCalculator_.compute(crc,&buffer[0],8);
    }
    const uint32_t remainingSize = it->iov_len - offset;
    overflow = remainingSize % 8;
    crcCalculator_.compute(crc,(uint8_t*)it->iov_base+offset,remainingSize-overflow);
    if ( overflow > 0 )
    {
      buffer.resize(8);
      memcpy(&buffer[0],(uint8_t*)it->iov_base+offset+remainingSize-overflow,overflow);
    }
  }

  fedTrailer->conscheck = conscheck;

  const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);
  if ( trailerCRC != crc )
  {
    if ( isCorrupted_ )
      errorMsg_ << ". ";
    errorMsg_ << "The CRC in the FED trailer claims 0x" << std::hex << trailerCRC;
    errorMsg_ << ", but recalculation gives 0x" << crc;
    hasCRCerror_ = true;
  }
}


void evb::readoutunit::FedFragment::checkTrailerBits(const uint32_t conscheck)
{
  if ( (conscheck & 0xC004) == 0) return;

  hasFEDerror_ = true;

  if ( isCorrupted_ || hasCRCerror_ )
    errorMsg_ << ". In addition, the";
  else
    errorMsg_ << "The";
  errorMsg_ << " FED trailer indicates that ";

  bool foundError = false;
  if ( conscheck & 0x4 ) // FED CRC error (R bit)
  {
    errorMsg_ << "a wrong FED CRC checksum was found by the FEROL (FED trailer R bit is set)";
    foundError = true;
  }
  if ( conscheck & 0x4000 ) // wrong FED id (F bit)
  {
    if ( foundError )
      errorMsg_ << ", and ";
    errorMsg_ << "the FED id is not expected by the FEROL (FED trailer F bit is set)";
    foundError = true;
  }
  if ( conscheck & 0x8000 ) // slink CRC error (C bit)
  {
    if ( foundError )
      errorMsg_ << ", and ";
    errorMsg_ << "wrong slink CRC checksum was found by the FEROL (FED trailer C bit is set)";
  }
}


void evb::readoutunit::FedFragment::reportErrors() const
{
  if ( isCorrupted_ )
  {
    std::ostringstream msg;
    msg << "Received a corrupted event " << eventNumber_ << " from FED " << fedId_ << ": ";
    msg << errorMsg_.str();
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }
  else if ( isOutOfSequence_ )
  {
    std::ostringstream msg;
    msg << "Received an event out of sequence from FED " << fedId_ << ": ";
    msg << errorMsg_.str();
    XCEPT_RAISE(exception::EventOutOfSequence, msg.str());
  }
  else if ( hasCRCerror_ && evb::isFibonacci( ++crcErrors_ ) )
  {
    std::ostringstream msg;
    msg << "Received " << crcErrors_ << " events with wrong CRC checksum from FED " << fedId_ << ": ";
    msg << errorMsg_.str();
    XCEPT_RAISE(exception::CRCerror, msg.str());
  }
  else if ( hasFEDerror_ && evb::isFibonacci( ++fedErrorCount_ ) )
  {
    std::ostringstream msg;
    msg << "Received " << fedErrorCount_ << " bad events from FED " << fedId_ << ": ";
    msg << errorMsg_.str();
    XCEPT_RAISE(exception::FEDerror, msg.str());
  }
}


void evb::readoutunit::FedFragment::dump
(
  std::ostream& s,
  const std::string& reasonForDump
)
{
  std::vector<unsigned char> buffer;
  uint32_t copiedSize = 0;
  if ( isComplete_ )
  {
    buffer.resize( getFedSize() );
    for ( DataLocations::const_iterator it = dataLocations_.begin(), itEnd = dataLocations_.end();
          it != itEnd; ++it)
    {
      memcpy(&buffer[copiedSize],it->iov_base,it->iov_len);
      copiedSize += it->iov_len;
    }
  }
  else
  {
    for ( SocketBuffers::const_iterator it = socketBuffers_.begin(), itEnd = socketBuffers_.end();
          it != itEnd; ++it)
    {
      toolbox::mem::Reference* bufRef = (*it)->getBufRef();
      buffer.resize( copiedSize + bufRef->getDataSize() );
      memcpy(&buffer[copiedSize],bufRef->getDataLocation(),bufRef->getDataSize());
      copiedSize += bufRef->getDataSize();
    }
  }

  s << "==================== DUMP ======================" << std::endl;
  s << "Reason for dump: " << reasonForDump << std::endl;
  s << "FED completely received   : " << (isComplete_?"true":"false") << std::endl;
  s << "Buffer size          (dec): " << copiedSize << std::endl;
  s << "FED size             (dec): " << fedSize_ << std::endl;
  s << "FED id               (dec): " << fedId_ << std::endl;
  s << "Trigger no           (dec): " << eventNumber_ << std::endl;
  if ( evbId_.isValid() )
    s << "EvB id                    : " << evbId_ << std::endl;

  if ( bufRef_ )
    DumpUtility::dumpBlockData(s, (unsigned char*)bufRef_->getDataLocation(), bufRef_->getDataSize());
  else
    DumpUtility::dumpBlockData(s, &buffer[0], copiedSize);

  s << "================ END OF DUMP ===================" << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
