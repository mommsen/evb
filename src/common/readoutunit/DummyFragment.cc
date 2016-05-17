#include "evb/Exception.h"
#include "evb/readoutunit/DummyFragment.h"


evb::readoutunit::DummyFragment::DummyFragment
(
  const uint16_t fedId,
  const bool isMasterFed,
  const uint32_t fedSize,
  const bool computeCRC,
  const std::string& subSystem,
  const EvBidFactoryPtr& evbIdFactory,
  const uint32_t checkCRC,
  uint32_t& fedErrorCount,
  uint32_t& crcErrors
)
  : FedFragment(fedId,isMasterFed,subSystem,evbIdFactory,checkCRC,fedErrorCount,crcErrors),
    remainingFedSize_(fedSize),
    computeCRC_(computeCRC),
    fedCRC_(0xffff)
{
  fedSize_ = fedSize;
  evbId_ = evbIdFactory_->getEvBid();
  eventNumber_ = evbId_.eventNumber();
  bxId_ = evbId_.bxId();
}


evb::readoutunit::DummyFragment::~DummyFragment()
{}


bool evb::readoutunit::DummyFragment::fillData(unsigned char* payload, const uint32_t remainingPayloadSize, uint32_t& copiedSize)
{
  copiedSize = 0;

  while ( remainingFedSize_ > 0 && remainingPayloadSize-copiedSize > 0 )
  {
    switch (typeOfNextComponent_)
    {
      case FEROL_HEADER:
      {
        // FEROL header is not filled into super-fragment
        typeOfNextComponent_ = FED_HEADER;
        break;
      }
      case FED_HEADER:
      {
        fedh_t* fedHeader;
        if ( tmpBufferSize_ > 0 )
        {
          // header is partially contained in the previous chunk
          const uint32_t missingHeaderBytes = sizeof(fedh_t)-tmpBufferSize_;
          memcpy(payload+copiedSize,&tmpBuffer_[tmpBufferSize_],missingHeaderBytes);
          copiedSize += missingHeaderBytes;
          tmpBufferSize_ = 0;
        }
        else if ( remainingPayloadSize-copiedSize < sizeof(fedh_t) )
        {
          // the header does not fully fit into this chunk
          if ( tmpBuffer_.size() < sizeof(fedh_t) )
            tmpBuffer_.resize( sizeof(fedh_t) );
          fedHeader = (fedh_t*)&tmpBuffer_[0];
          fillFedHeader(fedHeader);
          memcpy(payload+copiedSize,&tmpBuffer_[0],remainingPayloadSize-copiedSize);
          copiedSize += remainingPayloadSize-copiedSize;
          tmpBufferSize_ = remainingPayloadSize-copiedSize;
          break;
        }
        else
        {
          fedHeader = (fedh_t*)(payload+copiedSize);
          fillFedHeader(fedHeader);
          copiedSize += sizeof(fedh_t);
        }
        remainingFedSize_ -= sizeof(fedh_t);
        typeOfNextComponent_ = FED_PAYLOAD;
        break;
      }
      case FED_PAYLOAD:
      {
        uint32_t payloadSize;
        if ( (remainingFedSize_-sizeof(fedt_t)) > remainingPayloadSize-copiedSize )
        {
          payloadSize = remainingPayloadSize-copiedSize;
        }
        else
        {
          payloadSize = remainingFedSize_ - sizeof(fedt_t);
          typeOfNextComponent_ = FED_TRAILER;
        }

        memset(payload+copiedSize, 0xCA, payloadSize);

        if ( computeCRC_ )
          crcCalculator_.compute(fedCRC_,(uint8_t*)(payload+copiedSize),payloadSize);

        copiedSize += payloadSize;
        remainingFedSize_ -= payloadSize;
        break;
      }
      case FED_TRAILER:
      {
        fedt_t* fedTrailer;
        if ( tmpBufferSize_ > 0 )
        {
          // trailer is partially contained in the previous chunk
          const uint32_t missingTrailerBytes = sizeof(fedt_t)-tmpBufferSize_;
          memcpy(payload+copiedSize,&tmpBuffer_[tmpBufferSize_],missingTrailerBytes);
          copiedSize += missingTrailerBytes;
          tmpBufferSize_ = 0;
        }
        else if ( remainingPayloadSize-copiedSize < sizeof(fedt_t) )
        {
          // the trailer does not fully fit into this chunk
          if ( tmpBuffer_.size() < sizeof(fedt_t) )
            tmpBuffer_.resize( sizeof(fedt_t) );
          fedTrailer = (fedt_t*)&tmpBuffer_[0];
          fillFedTrailer(fedTrailer);
          memcpy(payload+copiedSize,&tmpBuffer_[0],remainingPayloadSize-copiedSize);
          copiedSize += remainingPayloadSize-copiedSize;
          tmpBufferSize_ = remainingPayloadSize-copiedSize;
          break;
        }
        else
        {
          fedTrailer = (fedt_t*)(payload+copiedSize);
          fillFedTrailer(fedTrailer);
          copiedSize += sizeof(fedt_t);
        }
        remainingFedSize_ -= sizeof(fedt_t);
        isComplete_ = true;
        break;
      }
    }
  }
  return isComplete_;
}


void evb::readoutunit::DummyFragment::fillFedHeader(fedh_t* fedHeader)
{
  fedHeader->sourceid = (evbId_.bxId() << FED_BXID_SHIFT) | (fedId_ << FED_SOID_SHIFT);
  fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | evbId_.eventNumber();

  if ( computeCRC_ )
    fedCRC_ = crcCalculator_.compute((uint8_t*)fedHeader,sizeof(fedh_t));
}


void evb::readoutunit::DummyFragment::fillFedTrailer(fedt_t* fedTrailer)
{
  fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) | (fedSize_ >> 3);

  if ( computeCRC_ )
  {
    // Force C,F,R & CRC field to zero before re-computing the CRC.
    // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
    fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);
    crcCalculator_.compute(fedCRC_,(uint8_t*)fedTrailer,sizeof(fedt_t));
    fedTrailer->conscheck |= (fedCRC_ << FED_CRCS_SHIFT);
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
