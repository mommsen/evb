#include <algorithm>
#include <sstream>
#include <string.h>
#include <sys/time.h>
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
  const uint32_t maxFedSize,
  const bool computeCRC
) :
  fedId_(fedId),
  fedSize_(fedSize),
  minFedSize_(minFedSize),
  maxFedSize_(maxFedSize),
  maxTriggerRate_(0),
  computeCRC_(computeCRC),
  fedCRC_(0xffff),
  typeOfNextComponent_(FED_HEADER),
  lastTime_(0),
  availableTriggers_(0)
{
  if (minFedSize < sizeof(fedh_t) + sizeof(fedt_t))
  {
    std::ostringstream msg;
    msg << "The minimal FED size in the configuration (minFedSize) must be at least ";
    msg << sizeof(fedh_t) + sizeof(fedt_t) << " Bytes instead of " << minFedSize << " Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  if (useLogNormal)
  {
    logNormalGen_.reset( new toolbox::math::LogNormalGen(time(0),fedSize,fedSizeStdDev) );
  }
}


uint32_t evb::FragmentTracker::startFragment(const EvBid& evbId)
{
  if ( typeOfNextComponent_ != FED_HEADER )
  {
    std::ostringstream msg;
    msg << "Request to start a new dummy FED fragment for evb id " << evbId;
    msg << " with FED " << fedId_;
    msg << ", while the previous fragment for evb id " << evbId_;
    msg << " is not yet complete";
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }

  waitForNextTrigger();

  evbId_ = evbId;
  currentFedSize_ = getFedSize();
  remainingFedSize_ = currentFedSize_;

  return currentFedSize_;
}


void evb::FragmentTracker::startRun()
{
  struct timeval time;
  gettimeofday(&time,0);
  lastTime_ = time.tv_sec + static_cast<double>(time.tv_usec) / 1000000;
  availableTriggers_ = 0;
}


void evb::FragmentTracker::waitForNextTrigger()
{
  if ( maxTriggerRate_ == 0 ) return;

  if ( availableTriggers_ > 0 )
  {
    --availableTriggers_;
    return;
  }

  struct timeval time;
  double now = 0;
  while ( availableTriggers_ == 0 )
  {
    gettimeofday(&time,0);
    now = time.tv_sec + static_cast<double>(time.tv_usec) / 1000000;
    if ( lastTime_ == 0 )
      availableTriggers_ = 1;
    else
      availableTriggers_ = static_cast<uint32_t>(now>lastTime_ ? (now-lastTime_)*maxTriggerRate_ : 0);
  }
  lastTime_ = now;
  --availableTriggers_;
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

        fedHeader->sourceid = (evbId_.bxId() << FED_BXID_SHIFT) | (fedId_ << FED_SOID_SHIFT);
        fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | evbId_.eventNumber();

        if ( computeCRC_ )
          fedCRC_ = crcCalculator_.compute(payload,sizeof(fedh_t));

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

        if ( computeCRC_ )
          crcCalculator_.compute(fedCRC_,payload,payloadSize);

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

        if ( computeCRC_ )
        {
          // Force C,F,R & CRC field to zero before re-computing the CRC.
          // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
          fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);
          crcCalculator_.compute(fedCRC_,payload,sizeof(fedt_t));
          fedTrailer->conscheck = (fedCRC_ << FED_CRCS_SHIFT);
        }

        payload += sizeof(fedt_t);
        bytesFilled += sizeof(fedt_t);
        nbBytesAvailable -= sizeof(fedt_t);
        remainingFedSize_ -= sizeof(fedt_t);

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
