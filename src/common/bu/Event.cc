#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdlib.h>
#include <sys/mman.h>
#include <sstream>
#include <zlib.h>

#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/ferol_header.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
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
  eventInfo_ = new EventInfo(evbId.runNumber(), evbId.lumiSection(), evbId.eventNumber());
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
  unsigned char* payload
)
{
  myBufRefs_.push_back(bufRef);

  const msg::SuperFragment* superFragmentMsg = (msg::SuperFragment*)payload;
  payload += superFragmentMsg->getHeaderSize();

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
    pos->second = superFragmentMsg->totalSize;
    eventInfo_->eventSize += superFragmentMsg->totalSize;
  }

  superFragmentMsg->appendFedIds(missingFedIds_);

  if (superFragmentMsg->partSize > 0)
  {
    DataLocationPtr dataLocation( new DataLocation(payload,superFragmentMsg->partSize) );
    dataLocations_.push_back(dataLocation);

    #ifdef EVB_DEBUG_CORRUPT_EVENT
    unsigned char* posToCorrupt = const_cast<unsigned char*>(payload + 23);
    if ( size_t(posToCorrupt) & 0x10 )
    {
      *posToCorrupt ^= 1;
    }
    #endif //EVB_DEBUG_CORRUPT_EVENT
  }

  // erase at the very end. Otherwise the event might be considered complete
  // before the last chunk has been fully treated
  pos->second -= superFragmentMsg->partSize;
  if ( pos->second == 0 )
  {
    ruSizes_.erase(pos);
    return true;
  }

  return false;
}


void evb::bu::Event::writeToDisk
(
  FileHandlerPtr fileHandler,
  const bool calculateAdler32
) const
{
  if ( dataLocations_.empty() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot find any FED data");
  }

  // Get the memory mapped file chunk
  const size_t bufferSize = eventInfo_->getBufferSize();
  unsigned char* map = (unsigned char*)fileHandler->getMemMap(bufferSize);

  // Write event
  unsigned char* filepos = map + sizeof(EventInfo);

  for (DataLocations::const_iterator it = dataLocations_.begin(), itEnd = dataLocations_.end();
       it != itEnd; ++it)
  {
    if ( calculateAdler32 )
      eventInfo_->adler32 = ::adler32(eventInfo_->adler32, (*it)->location, (*it)->length);

    memcpy(filepos, (*it)->location, (*it)->length);
    filepos += (*it)->length;
  }

  // Write event information
  memcpy(map, eventInfo_, sizeof(EventInfo));

  if ( munmap(map, bufferSize) == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to unmap the data file"
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}


void evb::bu::Event::checkEvent(const uint32_t checkCRC) const
{
  if ( ! isComplete() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot check an incomplete event for data integrity");
  }

  if ( ! missingFedIds_.empty() )
  {
    std::cout << "Event " << evbId_ << " is missing data from FEDs: ";
    for ( msg::FedIds::const_iterator it = missingFedIds_.begin(), itEnd = missingFedIds_.end();
          it != itEnd; ++it )
    {
      std::cout << *it << " ";
    }
    std::cout << std::endl;
  }

  DataLocations::const_reverse_iterator rit = dataLocations_.rbegin();
  const DataLocations::const_reverse_iterator ritEnd = dataLocations_.rend();
  uint32_t chunk = dataLocations_.size() - 1;
  std::set<uint32_t> fedIdsSeen;
  std::vector<std::string> crcErrors;
  const bool computeCRC = ( checkCRC > 0 && evbId_.eventNumber() % checkCRC == 0 );

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

      try
      {
        fedInfo.checkData(evbId_.eventNumber(), computeCRC);
      }
      catch(exception::CRCerror& e)
      {
        crcErrors.push_back(e.message());
      }
      if ( ! fedIdsSeen.insert( fedInfo.fedId() ).second )
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
  catch(exception::DataCorruption& e)
  {
    std::ostringstream oss;
    oss << "Found bad data in chunk " << chunk << " of event with EvB id " << evbId_ << ": " << std::endl;

    dumpEventToFile(e.message(),chunk);

    XCEPT_RETHROW(exception::DataCorruption, oss.str(), e);
  }

  if ( ! crcErrors.empty() )
  {
    std::ostringstream oss;
    oss << "Found CRC errors in event with EvB id " << evbId_ << ": " << std::endl;
    for ( std::vector<std::string>::const_iterator it = crcErrors.begin(), itEnd = crcErrors.end();
          it != itEnd; ++it)
    {
      oss << *it << std::endl;;
    }

    XCEPT_RAISE(exception::CRCerror, oss.str());
  }
}


void evb::bu::Event::dumpEventToFile(const std::string& reasonForDump, const uint32_t badChunk) const
{
  std::ostringstream fileName;
  fileName << "/tmp/dump_run" << std::setfill('0') << std::setw(6) << runNumber()
    << "_event" << std::setw(8) << eventNumber()
    << ".txt";
  std::ofstream dumpFile;
  dumpFile.open(fileName.str().c_str());
  dumpFile << "Reason for dump: " << reasonForDump << std::endl;

  for ( uint32_t i = 0; i < dataLocations_.size(); ++i )
  {
    if ( i == badChunk )
    {
      dumpFile << toolbox::toString("Bad chunk %2d: ", i);
      dumpFile << std::string(112,'*') << std::endl;
    }
    else
    {
      dumpFile << toolbox::toString("Chunk %2d : ",i);
      dumpFile << std::string(115,'-') << std::endl;
    }
    DumpUtility::dumpBlockData(dumpFile,dataLocations_[i]->location,dataLocations_[i]->length);
  }
  dumpFile.close();
}


evb::CRCCalculator evb::bu::Event::FedInfo::crcCalculator_;


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


void evb::bu::Event::FedInfo::checkData(const uint32_t eventNumber, const bool computeCRC) const
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

  if ( computeCRC )
  {
    // Force C,F,R & CRC field to zero before re-computing the CRC.
    // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
    const uint32_t conscheck = trailer()->conscheck;
    trailer()->conscheck &= ~(FED_CRCS_MASK | 0xC004);

    uint16_t crc(0xffff);
    for (DataLocations::const_reverse_iterator rit = fedData_.rbegin(), ritEnd = fedData_.rend();
         rit != ritEnd; ++rit)
    {
      crcCalculator_.compute(crc,(*rit)->location,(*rit)->length);
    }

    trailer()->conscheck = conscheck;
    const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);

    if ( trailerCRC != crc )
    {
      std::ostringstream oss;
      oss << "Wrong CRC checksum in FED trailer for FED " << fedId();
      oss << ": found 0x" << std::hex << trailerCRC;
      oss << ", but calculated 0x" << std::hex << crc;
      XCEPT_RAISE(exception::CRCerror, oss.str());
    }
  }
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
  const uint32_t lumi,
  const uint32_t event
) :
  version(3),
  runNumber(run),
  lumiSection(lumi),
  eventNumber(event),
  eventSize(0),
  paddingSize(0),
  adler32(::adler32(0L,Z_NULL,0))
{}


inline
size_t evb::bu::Event::EventInfo::getBufferSize()
{
  const size_t pageSize = sysconf(_SC_PAGE_SIZE);
  paddingSize = pageSize - (sizeof(EventInfo) + eventSize) % pageSize;
  return ( sizeof(EventInfo) + eventSize + paddingSize );
}


namespace evb{
  namespace bu {

    std::ostream& operator<<
    (
      std::ostream& str,
      const evb::bu::Event::EventInfo& eventInfo
    )
    {
      str << "EventInfo:";
      str << " version=" << eventInfo.version;
      str << " runNumber=" << eventInfo.runNumber;
      str << " lumiSection=" << eventInfo.lumiSection;
      str << " eventNumber=" << eventInfo.eventNumber;
      str << " adler32=0x" << std::hex << eventInfo.adler32 << std::dec;
      str << " eventSize=" << eventInfo.eventSize;
      str << " paddingSize=" << eventInfo.paddingSize;

      return str;
    }

  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
