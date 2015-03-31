#include <fstream>
#include <iomanip>
#include <limits>
#include <stdlib.h>
#include <sstream>

#include "evb/bu/Event.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "xcept/tools.h"


evb::bu::Event::Event
(
  const EvBid& evbId,
  const bool calculateAdler32,
  const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* dataBlockMsg
) :
  evbId_(evbId),
  calculateAdler32_(calculateAdler32),
  buResourceId_(dataBlockMsg->buResourceId)
{
  eventInfo_ = EventInfoPtr( new EventInfo(evbId.runNumber(), evbId.lumiSection(), evbId.eventNumber()) );
  msg::RUtids ruTids;
  dataBlockMsg->getRUtids(ruTids);
  for (uint32_t i = 0; i < dataBlockMsg->nbRUtids; ++i)
  {
    ruSizes_.insert( RUsizes::value_type(ruTids[i],std::numeric_limits<uint32_t>::max()) );
  }
}


evb::bu::Event::~Event()
{
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
  payload += superFragmentMsg->headerSize;

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
    eventInfo_->addFedSize(superFragmentMsg->totalSize);
  }

  superFragmentMsg->appendFedIds(missingFedIds_);

  if (superFragmentMsg->partSize > 0)
  {
    #ifdef EVB_DEBUG_CORRUPT_EVENT
    unsigned char* posToCorrupt = const_cast<unsigned char*>(payload + 23);
    if ( size_t(posToCorrupt) & 0x10 )
    {
      *posToCorrupt ^= 1;
    }
    #endif //EVB_DEBUG_CORRUPT_EVENT

    iovec dataLocation;
    dataLocation.iov_base = payload;
    dataLocation.iov_len = superFragmentMsg->partSize;
    dataLocations_.push_back(dataLocation);

    if (calculateAdler32_)
      eventInfo_->updateAdler32(dataLocation);
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


void evb::bu::Event::checkEvent(const uint32_t& checkCRC) const
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

  FedInfo::DataLocations::const_reverse_iterator rit = dataLocations_.rbegin();
  const FedInfo::DataLocations::const_reverse_iterator ritEnd = dataLocations_.rend();
  uint32_t chunk = dataLocations_.size() - 1;
  std::set<uint32_t> fedIdsSeen;
  std::vector<std::string> crcErrors;
  const bool computeCRC = ( checkCRC > 0 && evbId_.eventNumber() % checkCRC == 0 );

  try
  {
    uint32_t remainingLength = rit->iov_len;

    do {

      FedInfo fedInfo((unsigned char*)rit->iov_base, remainingLength);

      while ( ! fedInfo.complete() )
      {
        ++rit;
        if ( rit == ritEnd )
        {
          XCEPT_RAISE(exception::SuperFragment,"Corrupted superfragment: Premature end of data encountered");
        }
        remainingLength = rit->iov_len;
        --chunk;
        fedInfo.addDataChunk((unsigned char*)rit->iov_base, remainingLength);
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
          remainingLength = rit->iov_len;
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
  fileName << "/tmp/dump_run" << std::setfill('0') << std::setw(6) << eventInfo_->runNumber()
    << "_event" << std::setw(8) << eventInfo_->eventNumber()
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
    DumpUtility::dumpBlockData(dumpFile,(unsigned char*)dataLocations_[i].iov_base,dataLocations_[i].iov_len);
  }
  dumpFile.close();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
