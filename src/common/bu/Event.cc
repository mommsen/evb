#include <algorithm>
#include <limits>
#include <stdlib.h>
#include <sys/mman.h>
#include <sstream>

#include <boost/crc.hpp>

#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/ferol_header.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/CRC16.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "xcept/tools.h"


evb::bu::Event::Event
(
  const EvBid& evbId,
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg
) :
evbId_(evbId),
buResourceId_(dataBlockMsg->buResourceId)
{
  eventInfo_ = new EventInfo(evbId.runNumber(), evbId.eventNumber(), evbId.lumiSection());
  msg::RUtids ruTids;
  dataBlockMsg->getRUtids(ruTids);
  for (uint32_t i = 0; i < dataBlockMsg->nbRUtids; ++i)
  {
    ruSizes_.insert( RUsizes::value_type(ruTids[i],std::numeric_limits<uint32_t>::max()) );
  }
}


evb::bu::Event::~Event()
{
  delete eventInfo_;
  dataLocations_.clear();
  for (BufferReferences::iterator it = myBufRefs_.begin(), itEnd = myBufRefs_.end();
       it != itEnd; ++it)
  {
    (*it)->release();
  }
  myBufRefs_.clear();
}


bool evb::bu::Event::appendSuperFragment
(
  const I2O_TID ruTid,
  toolbox::mem::Reference* bufRef,
  const unsigned char* fragmentPos,
  const uint32_t partSize,
  const uint32_t totalSize
)
{
  const RUsizes::iterator pos = ruSizes_.find(ruTid);
  if ( pos == ruSizes_.end() )
  {
    std::ostringstream oss;
    oss << "Received a duplicated or unexpected super fragment";
    oss << " from RU TID " << ruTid;
    oss << " for EvB id " << evbId_;
    oss << " Outstanding messages from RU TIDs:";
    for (RUsizes::const_iterator it = ruSizes_.begin(), itEnd = ruSizes_.end();
         it != itEnd; ++it)
      oss << " " << it->first;
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  if ( pos->second == std::numeric_limits<uint32_t>::max() )
  {
    pos->second = totalSize;
    eventInfo_->eventSize += totalSize;
  }

  if (partSize == 0) return false;

  myBufRefs_.push_back(bufRef);
  DataLocationPtr dataLocation( new DataLocation(fragmentPos,partSize) );
  dataLocations_.push_back(dataLocation);

  // erase at the very end. Otherwise the event might be considered complete
  // before the last chunk has been fully treated
  pos->second -= partSize;
  if ( pos->second == 0 )
  {
    ruSizes_.erase(pos);
    return true;
  }

  return false;
}


void evb::bu::Event::writeToDisk(FileHandlerPtr fileHandler)
{
  if ( dataLocations_.empty() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot find any FED data");
  }

  // Get the memory mapped file chunk
  const size_t bufferSize = eventInfo_->getBufferSize();
  char* map = (char*)fileHandler->getMemMap(bufferSize);

  // Write event information
  memcpy(map, eventInfo_, eventInfo_->headerSize);

  // Write event
  char* filepos = map+eventInfo_->headerSize;

  for (DataLocations::const_iterator it = dataLocations_.begin(), itEnd = dataLocations_.end();
       it != itEnd; ++it)
  {
    memcpy(filepos, (*it)->location, (*it)->length);
    filepos += (*it)->length;
  }

  if ( munmap(map, bufferSize) == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to unmap the data file"
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileHandler->incrementEventCount();
}


void evb::bu::Event::checkEvent()
{
  if ( ! isComplete() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot check an incomplete event for data integrity");
  }

  DataLocations::const_reverse_iterator rit = dataLocations_.rbegin();
  const DataLocations::const_reverse_iterator ritEnd = dataLocations_.rend();
  uint32_t chunk = dataLocations_.size() - 1;

  try
  {
    uint32_t remainingLength = (*rit)->length;

    do {

      FedInfo fedInfo((*rit)->location, remainingLength);

      while ( ! fedInfo.complete() )
      {
        ++rit;
        if ( rit == ritEnd )
        {
          XCEPT_RAISE(exception::SuperFragment,"Corrupted superfragment: Premature end of data encountered");
        }
        remainingLength = (*rit)->length;
        --chunk;
        fedInfo.addDataChunk((*rit)->location, remainingLength);
      }

      fedInfo.checkData( evbId_.eventNumber() );

      if ( !eventInfo_->addFedSize(fedInfo) )
      {
        std::ostringstream oss;
        oss << "Found a duplicated FED id " << fedInfo.fedId();
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }

      if ( remainingLength == 0 )
      {
        // the next trailer is in the next data chunk
        if ( ++rit != ritEnd )
        {
          remainingLength = (*rit)->length;
          --chunk;
        }
      }

    } while ( rit != ritEnd );
  }
  catch(xcept::Exception& e)
  {
    std::ostringstream oss;

    oss << "Found bad data in chunk " << chunk << " of event with EvB id " << evbId_ << ": " << std::endl;
    for ( uint32_t i = 0; i < dataLocations_.size(); ++i )
    {
      if ( i == chunk )
      {
        oss << toolbox::toString("Bad chunk %2d: ", i);
        oss << std::string(112,'*') << std::endl;
      }
      else
      {
        oss << toolbox::toString("Chunk %2d : ",i);
        oss << std::string(115,'-') << std::endl;
      }
      DumpUtility::dumpBlockData(oss,dataLocations_[i]->location,dataLocations_[i]->length);
    }

    XCEPT_RETHROW(exception::DataCorruption, oss.str(), e);
  }
}


evb::bu::Event::FedInfo::FedInfo(const unsigned char* pos, uint32_t& remainingLength)
{
  fedt_t* trailer = (fedt_t*)(pos + remainingLength - sizeof(fedt_t));

  if ( FED_TCTRLID_EXTRACT(trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    std::ostringstream oss;
    oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    oss << " but got event size 0x" << std::hex << trailer->eventsize;
    oss << " and conscheck 0x" << std::hex << trailer->conscheck;
    oss << " at offset 0x" << std::hex << remainingLength;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  const uint32_t fedSize = FED_EVSZ_EXTRACT(trailer->eventsize)<<3;
  const uint32_t length = std::min(fedSize,remainingLength);
  remainingFedSize_ = fedSize - length;
  remainingLength -= length;

  fedData_.push_back(DataLocationPtr( new DataLocation(pos+remainingLength,length) ));
}


void evb::bu::Event::FedInfo::addDataChunk(const unsigned char* pos, uint32_t& remainingLength)
{
  const uint32_t length = std::min(remainingFedSize_,remainingLength);
  remainingFedSize_ -= length;
  remainingLength -= length;
  fedData_.push_back(DataLocationPtr( new DataLocation(pos+remainingLength,length) ));
}


void evb::bu::Event::FedInfo::checkData(const uint32_t eventNumber)
{
  if ( FED_HCTRLID_EXTRACT(header()->eventid) != FED_SLINK_START_MARKER )
  {
    std::ostringstream oss;
    oss << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
    oss << " but got event id 0x" << std::hex << header()->eventid;
    oss << " and source id 0x" << std::hex << header()->sourceid;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  if (eventId() != eventNumber)
  {
    std::ostringstream oss;
    oss << "FED header \"eventid\" " << eventId() << " does not match";
    oss << " expected eventNumber " << eventNumber;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  if ( fedId() >= FED_COUNT )
  {
    std::ostringstream oss;
    oss << "The FED id " << fedId() << " is larger than the maximum " << FED_COUNT;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  // recheck the trailer to assure that the fedData is correct
  if ( FED_TCTRLID_EXTRACT(trailer()->eventsize) != FED_SLINK_END_MARKER )
  {
    std::ostringstream oss;
    oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
    oss << " but got event size 0x" << std::hex << trailer()->eventsize;
    oss << " and conscheck 0x" << std::hex << trailer()->conscheck;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }

  #ifdef EVB_CALCULATE_CRC
  // Force CRC field to zero before re-computing the CRC.
  // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
  const uint32_t conscheck = trailer()->conscheck;
  trailer()->conscheck = 0;

  uint16_t crc(0xffff);
  for (DataLocations::const_reverse_iterator rit = fedData_.rbegin(), ritEnd = fedData_.rend();
       rit != ritEnd; ++rit)
  {
    computeCRC(crc,(*rit)->location,(*rit)->length);
  }

  trailer()->conscheck = conscheck;
  const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);

  if ( trailerCRC != crc )
  {
    std::ostringstream oss;
    oss << "Wrong CRC checksum in FED trailer for FED " << fedId();
    oss << ": found 0x" << std::hex << trailerCRC;
    oss << ", but calculated 0x" << std::hex << crc;
    XCEPT_RAISE(exception::DataCorruption, oss.str());
  }
  #endif
}


inline
fedh_t* evb::bu::Event::FedInfo::header() const
{
  return (fedh_t*)(fedData_.back()->location);
}


inline
fedt_t* evb::bu::Event::FedInfo::trailer() const
{
  const DataLocationPtr loc = fedData_.front();
  return (fedt_t*)(loc->location + loc->length - sizeof(fedt_t));
}


inline
evb::bu::Event::EventInfo::EventInfo
(
  const uint32_t run,
  const uint32_t event,
  const uint32_t lumi
) :
version(3),
runNumber(run),
eventNumber(event),
lumiSection(lumi),
eventSize(0)
{
  for (uint16_t i=0; i<FED_COUNT; ++i)
    fedSizes[i] = 0;
}


inline
bool evb::bu::Event::EventInfo::addFedSize(const FedInfo& fedInfo)
{
  const uint16_t fedId = fedInfo.fedId();
  if ( fedSizes[fedId] > 0 ) return false;

  const uint32_t fedSize = fedInfo.fedSize();
  fedSizes[fedId] = fedSize;

  return true;
}


inline
size_t evb::bu::Event::EventInfo::getBufferSize()
{
  const size_t pageSize = sysconf(_SC_PAGE_SIZE);
  paddingSize = pageSize - (headerSize + eventSize) % pageSize;
  return ( headerSize + eventSize + paddingSize );
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
