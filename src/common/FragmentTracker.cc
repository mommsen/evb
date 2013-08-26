#include <algorithm>
#include <sstream>
#include <string.h>
#include <time.h>

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "evb/Exception.h"
#include "evb/FragmentTracker.h"


evb::FragmentTracker::FragmentTracker
(
  const uint32_t fedId,
  const uint32_t fedSize,
  const bool useLogNormal,
  const uint32_t fedSizeStdDev,
  const uint32_t minFedSize,
  const uint32_t maxFedSize
) :
fedId_(fedId),
fedSize_(fedSize),
minFedSize_(minFedSize),
maxFedSize_(maxFedSize),
fedCRC_(0xffff),
typeOfNextComponent_(FED_HEADER)
{
  if (useLogNormal)
  {
    logNormalGen_.reset( new toolbox::math::LogNormalGen(time(0),fedSize,fedSizeStdDev) );
  }
}


uint32_t evb::FragmentTracker::startFragment(const uint32_t eventNumber)
{
  if ( typeOfNextComponent_ != FED_HEADER )
  {
    std::ostringstream oss;
    oss << "Request to start a new dummy FED fragment for FED id " << fedId_;
    oss << " while the previous fragment is not yet complete";
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  eventNumber_ = eventNumber;
  currentFedSize_ = getFedSize();
  remainingFedSize_ = currentFedSize_;

  return currentFedSize_;
}


uint32_t evb::FragmentTracker::getFedSize() const
{
  uint32_t fedSize = fedSize_;
  if ( logNormalGen_ )
  {
    fedSize = std::max((uint32_t)logNormalGen_->getRawRandomSize(), minFedSize_);
    if ( maxFedSize_ > 0 && fedSize > maxFedSize_ ) fedSize = maxFedSize_;
  }
  return fedSize & ~0x7;
}


size_t evb::FragmentTracker::fillData
(
  unsigned char* payload,
  size_t nbBytesAvailable
)
{
  fedh_t* fedHeader = 0;
  fedt_t* fedTrailer = 0;
  size_t payloadSize = 0;
  size_t bytesFilled = 0;

  while ( nbBytesAvailable > 0 )
  {
    switch (typeOfNextComponent_)
    {
      case FED_HEADER:
        if ( nbBytesAvailable < sizeof(fedh_t) ) return bytesFilled;

        fedHeader = (fedh_t*)payload;

        fedHeader->sourceid = fedId_ << FED_SOID_SHIFT;
        fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | eventNumber_;

        fedCRC_ = crcCalculator_.computeCRC(payload,sizeof(fedh_t));
        payload += sizeof(fedh_t);
        bytesFilled += sizeof(fedh_t);
        nbBytesAvailable -= sizeof(fedh_t);
        remainingFedSize_ -= sizeof(fedh_t);

        typeOfNextComponent_ = FED_PAYLOAD;
        break;

      case FED_PAYLOAD:
        if ( nbBytesAvailable < 8 ) return bytesFilled; // Minium payload size is 8 bytes

        if ( (remainingFedSize_-sizeof(fedt_t)) > nbBytesAvailable )
        {
          // Payload must always be a multiple of 8 bytes
          payloadSize = nbBytesAvailable - (nbBytesAvailable % 8);
        }
        else
        {
          payloadSize = remainingFedSize_ - sizeof(fedt_t);
          typeOfNextComponent_ = FED_TRAILER;
        }

        memset(payload, 0xCA, payloadSize);
        crcCalculator_.computeCRC(fedCRC_,payload,payloadSize);

        payload += payloadSize;
        bytesFilled += payloadSize;
        nbBytesAvailable -= payloadSize;
        remainingFedSize_ -= payloadSize;

        break;

      case FED_TRAILER:
        if ( nbBytesAvailable < sizeof(fedt_t) ) return bytesFilled;

        fedTrailer = (fedt_t*)payload;

        fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) |
          (currentFedSize_ >> 3);

        // Force CRC & R field to zero before re-computing the CRC.
        // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
        fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0x4);
        crcCalculator_.computeCRC(fedCRC_,payload,sizeof(fedt_t));
        fedTrailer->conscheck = (fedCRC_ << FED_CRCS_SHIFT);

        payload += sizeof(fedt_t);
        bytesFilled += sizeof(fedt_t);
        nbBytesAvailable -= sizeof(fedt_t);

        typeOfNextComponent_ = FED_HEADER;
        break;

      default:
        XCEPT_RAISE(exception::DummyData, "Unknown component type");
    }
  }
  return bytesFilled;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
