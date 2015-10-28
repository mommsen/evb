#include <sstream>

#include "evb/bu/FedInfo.h"
#include "evb/Constants.h"
#include "evb/Exception.h"
#include "xcept/tools.h"


evb::CRCCalculator evb::bu::FedInfo::crcCalculator_;


evb::bu::FedInfo::FedInfo(const unsigned char* pos, uint32_t& remainingLength)
{
  trailer_ = (fedt_t*)(pos + remainingLength - sizeof(fedt_t));

  const uint32_t fedSize = FED_EVSZ_EXTRACT(trailer_->eventsize)<<3;
  const uint32_t length = std::min(fedSize,remainingLength);
  remainingFedSize_ = fedSize - length;
  remainingLength -= length;

  iovec dataLocation;
  dataLocation.iov_base = (void*)(pos+remainingLength);
  dataLocation.iov_len = length;
  fedData_.push_back(dataLocation);

  if ( complete() )
    header_ = (fedh_t*)(pos+remainingLength);
  else
    header_ = 0;
}


void evb::bu::FedInfo::addDataChunk(const unsigned char* pos, uint32_t& remainingLength)
{
  const uint32_t length = std::min(remainingFedSize_,remainingLength);
  remainingFedSize_ -= length;
  remainingLength -= length;
  iovec dataLocation;
  dataLocation.iov_base = (void*)(pos+remainingLength);
  dataLocation.iov_len = length;
  fedData_.push_back(dataLocation);

  if ( complete() )
    header_ = (fedh_t*)(pos+remainingLength);
}


void evb::bu::FedInfo::checkData(const uint32_t eventNumber, const bool computeCRC) const
{
  if ( FED_HCTRLID_EXTRACT(header_->eventid) != FED_SLINK_START_MARKER )
  {
    std::ostringstream msg;
    msg << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
    msg << " but got event id 0x" << std::hex << header_->eventid;
    msg << " and source id 0x" << std::hex << header_->sourceid;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( eventId() != eventNumber )
  {
    std::ostringstream msg;
    msg << "FED header \"eventid\" " << eventId() << " does not match";
    msg << " expected eventNumber " << eventNumber;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( fedId() >= FED_COUNT )
  {
    std::ostringstream msg;
    msg << "The FED " << fedId() << " is larger than the maximum " << FED_COUNT;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( FED_TCTRLID_EXTRACT(trailer_->eventsize) != FED_SLINK_END_MARKER )
  {
    std::ostringstream msg;
    msg << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    msg << " but got event size 0x" << std::hex << trailer_->eventsize;
    msg << " and conscheck 0x" << std::hex << trailer_->conscheck;
    XCEPT_RAISE(exception::DataCorruption, msg.str());
  }

  if ( computeCRC )
  {
    // Force C,F,R & CRC field to zero before re-computing the CRC.
    // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
    const uint32_t conscheck = trailer_->conscheck;
    trailer_->conscheck &= ~(FED_CRCS_MASK | 0xC004);

    uint16_t crc(0xffff);
    for (DataLocations::const_reverse_iterator rit = fedData_.rbegin(), ritEnd = fedData_.rend();
         rit != ritEnd; ++rit)
    {
      crcCalculator_.compute(crc,(uint8_t*)rit->iov_base,rit->iov_len);
    }

    trailer_->conscheck = conscheck;
    const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);

    if ( trailerCRC != crc )
    {
      std::ostringstream msg;
      msg << "Wrong CRC checksum in FED trailer for FED " << fedId();
      msg << ": found 0x" << std::hex << trailerCRC;
      msg << ", but calculated 0x" << std::hex << crc;
      XCEPT_RAISE(exception::CRCerror, msg.str());
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
