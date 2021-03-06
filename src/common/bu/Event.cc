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
  const msg::RUtids& ruTids,
  const uint16_t buResourceId,
  const bool checkCRC,
  const bool calculateCRC32
) :
  evbId_(evbId),
  checkCRC_(checkCRC),
  calculateCRC32_(calculateCRC32),
  buResourceId_(buResourceId)
{
  eventInfo_ = EventInfoPtr( new EventInfo(evbId.runNumber(), evbId.lumiSection(), evbId.eventNumber()) );
  for ( msg::RUtids::const_iterator it = ruTids.begin(), itEnd = ruTids.end();
        it != itEnd; ++it)
  {
    ruSizes_.insert( RUsizes::value_type(*it,std::numeric_limits<uint32_t>::max()) );
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
    std::ostringstream msg;
    msg << "Received a duplicated or unexpected super fragment";
    msg << " from RU TID " << ruTid;
    msg << " for EvB id " << evbId_;
    msg << " Outstanding messages from RU TIDs:";
    for (RUsizes::const_iterator it = ruSizes_.begin(), itEnd = ruSizes_.end();
         it != itEnd; ++it)
      msg << " " << it->first;
    XCEPT_RAISE(exception::SuperFragment, msg.str());
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

    if (calculateCRC32_)
      eventInfo_->updateCRC32(dataLocation);
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


void evb::bu::Event::checkEvent() const
{
  if ( ! isComplete() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot check an incomplete event for data integrity");
  }

  DataLocations::const_reverse_iterator rit = dataLocations_.rbegin();
  const DataLocations::const_reverse_iterator ritEnd = dataLocations_.rend();
  uint32_t chunk = dataLocations_.size() - 1;
  std::set<uint32_t> fedIdsSeen;
  std::vector<std::string> crcErrors;

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
        fedInfo.checkData(evbId_.eventNumber(), checkCRC_);
      }
      catch(exception::CRCerror& e)
      {
        crcErrors.push_back(e.message());
      }
      if ( ! fedIdsSeen.insert( fedInfo.fedId() ).second )
      {
        std::ostringstream msg;
        msg << "Found a duplicated FED " << fedInfo.fedId();
        XCEPT_RAISE(exception::DataCorruption, msg.str());
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
    std::ostringstream msg;
    msg << "Found bad data in chunk " << chunk << " of event with EvB id " << evbId_ << ": " << std::endl;

    dumpEventToFile(e.message(),chunk);

    XCEPT_RETHROW(exception::DataCorruption, msg.str(), e);
  }

  if ( ! crcErrors.empty() )
  {
    std::ostringstream msg;
    msg << "Found CRC errors in event with EvB id " << evbId_ << ": " << std::endl;
    for ( std::vector<std::string>::const_iterator it = crcErrors.begin(), itEnd = crcErrors.end();
          it != itEnd; ++it)
    {
      msg << *it << std::endl;;
    }

    XCEPT_RAISE(exception::CRCerror, msg.str());
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
