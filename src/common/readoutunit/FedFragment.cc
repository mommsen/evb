#include "evb/Constants.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "evb/readoutunit/FedFragment.h"
//#include "interface/shared/i2ogevb2g.h"


evb::CRCCalculator evb::readoutunit::FedFragment::crcCalculator_;

evb::readoutunit::FedFragment::FedFragment
(
  const uint16_t fedId,
  const bool isMasterFed,
  const std::string& subSystem,
  const EvBidFactoryPtr& evbIdFactory,
  const uint32_t checkCRC,
  uint32_t& fedErrorCount,
  uint32_t& crcErrors
)
  : typeOfNextComponent_(FEROL_HEADER),
    fedId_(fedId),
    bxId_(FED_BXID_WIDTH+1),
    eventNumber_(0),
    fedSize_(0),
    isComplete_(false),
    tmpBufferSize_(0),
    evbIdFactory_(evbIdFactory),
    checkCRC_(checkCRC),
    fedErrorCount_(fedErrorCount),crcErrors_(crcErrors),
    isMasterFed_(isMasterFed),
    subSystem_(subSystem),
    isCorrupted_(false),isOutOfSequence_(false),
    hasCRCerror_(false),hasFEDerror_(false),
    bufRef_(0),copyOffset_(0)
{}


evb::readoutunit::FedFragment::~FedFragment()
{
  toolbox::mem::Reference* nextBufRef;
  while ( bufRef_ )
  {
    nextBufRef = bufRef_->getNextReference();
    bufRef_->setNextReference(0);

    if ( socketBuffers_.empty() )
      bufRef_->release();

    bufRef_ = nextBufRef;
  };
}


bool evb::readoutunit::FedFragment::fillData(unsigned char* payload, const uint32_t remainingPayloadSize, uint32_t& copiedSize)
{
  assert( isComplete_ );

  copiedSize = 0;

  while ( copyIterator_ != dataLocations_.end() )
  {
    const unsigned char* chunkBase  = (unsigned char*)copyIterator_->iov_base;
    const uint32_t chunkSize = copyIterator_->iov_len;

    if ( chunkSize-copyOffset_ <= remainingPayloadSize-copiedSize )
    {
      // fill the remaining fragment
      memcpy(payload+copiedSize, chunkBase+copyOffset_, chunkSize-copyOffset_);
      copiedSize += chunkSize-copyOffset_;
      ++copyIterator_;
      copyOffset_ = 0;
    }
    else
    {
      // fill the remaining payload
      memcpy(payload+copiedSize, chunkBase+copyOffset_, remainingPayloadSize-copiedSize);
      copyOffset_ += remainingPayloadSize-copiedSize;
      copiedSize = remainingPayloadSize;
      return false;
    }
  }
  return true;
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
          // previous buffer contained part of this trailer
          const uint32_t missingTrailerBytes = sizeof(fedt_t)-tmpBufferSize_;
          memcpy(&tmpBuffer_[tmpBufferSize_],pos+usedSize,missingTrailerBytes);
          usedSize += missingTrailerBytes;
          dataLocation.iov_len += missingTrailerBytes;
          remainingBufferSize -= missingTrailerBytes;
          tmpBufferSize_ = 0;
          fedTrailer = (fedt_t*)&tmpBuffer_[0];
        }
        else if ( remainingBufferSize < sizeof(fedt_t) )
        {
          // the trailer is not fully contained in this buffer
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
        copyIterator_ = dataLocations_.begin();

        checkFedTrailer(fedTrailer);

        try
        {
          if ( !evbId_.isValid() )
            evbId_ = evbIdFactory_->getEvBid(eventNumber_, bxId_, dataLocations_);
        }
        catch(exception::EventOutOfSequence& e)
        {
          isOutOfSequence_ = true;
          if ( ! errorMsg_.empty() )
            errorMsg_ += ". ";
          errorMsg_ += e.message();
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
    msg << " (" << subSystem_ << ")";
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( ferolHeader->is_first_packet() )
  {
    eventNumber_ = ferolHeader->event_number();
  }
  else if ( eventNumber_ != ferolHeader->event_number() )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    msg << "mismatch of event number in FEROL header:";
    msg << " expected " << eventNumber_ << ", but got " << ferolHeader->event_number();
    errorMsg_ += msg.str();
  }

  if ( fedId_ != ferolHeader->fed_id() )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    if ( ! errorMsg_.empty() )
      msg << ", and ";
    msg << "mismatch of FED id in FEROL header:";
    msg << " expected FED " << fedId_ << " (" << subSystem_ << "), but got " << ferolHeader->fed_id();
    errorMsg_ += msg.str();
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
    msg << " (" << subSystem_ << ")";
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  const uint32_t eventId = FED_LVL1_EXTRACT(fedHeader->eventid);
  if ( eventId != eventNumber_ )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    if ( ! errorMsg_.empty() )
      msg << ", and ";
    msg << "FED header \"eventid\" " << eventId;
    msg << " does not match the eventNumber found in FEROL header";
    errorMsg_ += msg.str();
  }

  const uint32_t sourceId = FED_SOID_EXTRACT(fedHeader->sourceid);
  if ( sourceId != fedId_ )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    if ( ! errorMsg_.empty() )
      msg << ", and ";
    msg << "FED header \"sourceId\" " << sourceId;
    msg << " does not match the FED " << fedId_ << " (" << subSystem_ << ") found in FEROL header";
    errorMsg_ += msg.str();
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
    msg << " (" << subSystem_ << ")";
    msg << " with expected size " << fedSize_ << " Bytes";
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  const uint32_t evsz = FED_EVSZ_EXTRACT(fedTrailer->eventsize)<<3;
  if ( evsz != fedSize_ )
  {
    isCorrupted_ = true;
    std::ostringstream msg;
    if ( ! errorMsg_.empty() )
      msg << ", and ";
    msg << "inconsistent event size:";
    msg << " FED trailer claims " << evsz << " Bytes,";
    msg << " while sum of FEROL headers yield " << fedSize_;
    errorMsg_ += msg.str();
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
    hasCRCerror_ = true;
    std::ostringstream msg;
    if ( ! errorMsg_.empty() )
      msg << ". ";
    msg << "The CRC in the FED trailer claims 0x" << std::hex << trailerCRC;
    msg << ", but recalculation gives 0x" << crc;
    errorMsg_ += msg.str();
  }
}


void evb::readoutunit::FedFragment::checkTrailerBits(const uint32_t conscheck)
{
  if ( (conscheck & 0xC004) == 0) return;

  std::ostringstream msg;
  if ( ! errorMsg_.empty() )
    msg << ".  In addition, the";
  else
    msg << "The";
  msg << " FED trailer indicates that ";

  if ( conscheck & 0x4 ) // FED CRC error (R bit)
  {
    msg << "a wrong FED CRC checksum was found by the FEROL (FED trailer R bit is set)";
    hasFEDerror_ = true;
  }
  if ( conscheck & 0x4000 ) // wrong FED id (F bit)
  {
    if ( hasFEDerror_ )
      msg << ", and ";
    msg << "the FED id is not expected by the FEROL (FED trailer F bit is set)";
    hasFEDerror_ = true;
  }
  if ( conscheck & 0x8000 ) // slink CRC error (C bit)
  {
    if ( hasFEDerror_ )
      msg << ", and ";
    msg << "wrong slink CRC checksum was found by the FEROL (FED trailer C bit is set)";
    hasFEDerror_ = true;
  }
  errorMsg_ += msg.str();
}


void evb::readoutunit::FedFragment::reportErrors() const
{
  if ( isCorrupted_ )
  {
    std::ostringstream msg;
    msg << "Received a corrupted event " << eventNumber_ << " from FED " << fedId_ << " (" << subSystem_ << "): ";
    msg << errorMsg_;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }
  else if ( isOutOfSequence_ )
  {
    std::ostringstream msg;
    msg << "Received an event out of sequence from FED " << fedId_ << " (" << subSystem_ << "): ";
    msg << errorMsg_;
    XCEPT_RAISE(exception::EventOutOfSequence, msg.str());
  }
  else if ( hasCRCerror_ && evb::isFibonacci( ++crcErrors_ ) )
  {
    std::ostringstream msg;
    msg << "Received " << crcErrors_ << " events with wrong CRC checksum from FED " << fedId_ << " (" << subSystem_ << "): ";
    msg << errorMsg_;
    XCEPT_RAISE(exception::CRCerror, msg.str());
  }
  else if ( hasFEDerror_ && evb::isFibonacci( ++fedErrorCount_ ) )
  {
    std::ostringstream msg;
    msg << "Received " << fedErrorCount_ << " bad events from FED " << fedId_ << " (" << subSystem_ << "): ";
    msg << errorMsg_;
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
  s << "SubSystem                 : " << subSystem_ << std::endl;
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
