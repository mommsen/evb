#include <algorithm>
#include <stdlib.h>
#include <sys/mman.h>
#include <sstream>

//#include <boost/crc.hpp> 

#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/CRC16.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "xcept/tools.h"


evb::bu::Event::Event
(
  const uint32_t runNumber,
  const uint32_t eventNumber,
  const uint32_t resourceId,
  const std::vector<I2O_TID>& ruTids
) :
resourceId_(resourceId),
ruTids_(ruTids)
{  
  eventInfo_ = new EventInfo(runNumber, eventNumber);
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


void evb::bu::Event::appendSuperFragment
(
  const I2O_TID ruTid,
  toolbox::mem::Reference* bufRef,
  unsigned char* fragmentPos,
  uint32_t length
)
{
  const RUtids::iterator pos =
    std::find(ruTids_.begin(),ruTids_.end(),ruTid);
  if ( pos == ruTids_.end() )
  {
    std::ostringstream oss;
    oss << "Received a duplicated or unexpected super fragment from RU TID " << ruTid;
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  myBufRefs_.push_back(bufRef);
  DataLocation dataLocation(fragmentPos,length);
  dataLocations_.push_back(dataLocation);
  
  // erase at the very end. Otherwise the event might be considered complete
  // before the last chunk has been fully treated
  ruTids_.erase(pos);
}


void evb::bu::Event::writeToDisk(FileHandlerPtr fileHandler)
{
  if ( dataLocations_.empty() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot find any FED data.");
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
    memcpy(filepos, it->location, it->length);
    filepos += it->length;
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
    XCEPT_RAISE(exception::EventOrder, "Cannot check an incomplete event for data integrity.");
  }
  
  DataLocations::const_reverse_iterator rit = dataLocations_.rbegin();
  const DataLocations::const_reverse_iterator ritEnd = dataLocations_.rend();

  uint32_t remainingLength = rit->length;
  
  do {
    FedInfo fedInfo;
    checkFedTrailer(rit->location, remainingLength, fedInfo);

    uint32_t remainingFedSize = fedInfo.fedSize();

    // now start the odyssee to the fragment header
    // advance to the data section where we expect the header
    while ( remainingFedSize > rit->length )
    {
      // advance to the next block
      remainingFedSize -= rit->length;
      ++rit;
      if ( rit == ritEnd )
      {
        XCEPT_RAISE(exception::SuperFragment,"Corrupted superfragment: Premature end of data encountered.");
      }
      remainingLength = rit->length;
    }
    
    remainingLength -= remainingFedSize;
    //fedInfo.crc = updateCRC(fedLocations_.size()-1, lastFragment);

    checkFedHeader(rit->location, remainingLength, fedInfo);
    if ( !eventInfo_->addFedSize(fedInfo) )
    {
      std::stringstream oss;
      
      oss << "Duplicated FED id " << fedInfo.fedId;
      oss << " found in event " << eventInfo_->eventNumber;
      
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
    
    if ( fedInfo.fedId == 512 /* TRIGGER_FED_ID */ )
    {
      // extract L1 information
    }

    if ( remainingLength == 0 ) ++rit; // the next trailer is in the next data chunk
    
  } while ( rit != ritEnd );
}


uint16_t evb::bu::Event::updateCRC
(
  const size_t& first,
  const size_t& last
) const
{
  //boost::crc_optimal<16, 0x8005, 0xFFFF, 0, false, false> crc;
  uint16_t crc(0xFFFF);

  // for (size_t i = first; i != last; --i)
  // {
  //   const FedLocationPtr loc = fedLocations_[i];
  //   const size_t wordCount = loc->length/8;
  //   for (size_t w=0; w<wordCount; ++w)
  //   {
  //     //crc.process_block(&loc->location[w*8+7],&loc->location[w*8]);
  //     for (int b=7; b >= 0; --b)
  //     {
  //       const unsigned char index = (crc >> 8) ^ loc->location[w*8+b];
  //       crc <<= 8;
  //       crc ^= crc_table[index];
  //     }
  //   }
  // }
  
  return crc; //.checksum();
}

void evb::bu::Event::checkFedHeader
(
  const unsigned char* pos,
  const uint32_t offset,
  FedInfo& fedInfo
) const
{
  const fedh_t* fedHeader = (fedh_t*)(pos + offset);
  
  if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
  {
    std::stringstream oss;
    
    oss << "Expected FED header of event " << eventInfo_->eventNumber;
    oss << " but got event id 0x" << std::hex << fedHeader->eventid;
    oss << " and source id 0x" << std::hex << fedHeader->sourceid;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  const uint32_t eventid = FED_LVL1_EXTRACT(fedHeader->eventid);
  fedInfo.fedId = FED_SOID_EXTRACT(fedHeader->sourceid);

  if (eventid != eventInfo_->eventNumber)
  {
    std::stringstream oss;
    
    oss << "FED header \"eventid\" " << eventid << " does not match";
    oss << " expected eventNumber " << eventInfo_->eventNumber;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  if ( fedInfo.fedId >= FED_COUNT )
  {
    std::stringstream oss;
    
    oss << "The FED id " << fedInfo.fedId << " is larger than the maximum " << FED_COUNT;
    oss << " in event " << eventid;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  checkCRC(fedInfo, eventid);
}


void evb::bu::Event::checkFedTrailer
(
  const unsigned char* pos,
  const uint32_t offset,
  FedInfo& fedInfo
) const
{
  fedInfo.trailer = (fedt_t*)(pos + offset - sizeof(fedt_t));
  
  if ( FED_TCTRLID_EXTRACT(fedInfo.trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    std::stringstream oss;
    
    oss << "Expected FED trailer for event " << eventInfo_->eventNumber;
    oss << " but got event size 0x" << std::hex << fedInfo.trailer->eventsize;
    oss << " and conscheck 0x" << std::hex << fedInfo.trailer->conscheck;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  // Force CRC field to zero before re-computing the CRC.
  // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
  fedInfo.conscheck = fedInfo.trailer->conscheck;
  fedInfo.trailer->conscheck = 0;
}  


void evb::bu::Event::checkCRC
(
  FedInfo& fedInfo,
  const uint32_t eventNumber
) const
{
  const uint16_t trailerCRC = FED_CRCS_EXTRACT(fedInfo.conscheck);
  fedInfo.trailer->conscheck = fedInfo.conscheck;

  if ( fedInfo.crc != 0xffff && trailerCRC != fedInfo.crc )
  {
    std::stringstream oss;
    
    oss << "Wrong CRC checksum in FED trailer of event " << eventNumber;
    oss << " for FED " << fedInfo.fedId;
    oss << ": found 0x" << std::hex << trailerCRC;
    oss << ", but calculated 0x" << std::hex << fedInfo.crc;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
}


inline
evb::bu::Event::EventInfo::EventInfo
(
  const uint32_t run,
  const uint32_t event
) :
version(3),
runNumber(run),
eventNumber(event),
lumiSection(0),
eventSize(0)
{
  for (uint16_t i=0; i<FED_COUNT; ++i)
    fedSizes[i] = 0;
  
  updatePaddingSize();
}


inline
bool evb::bu::Event::EventInfo::addFedSize(const FedInfo& fedInfo)
{
  if ( fedSizes[fedInfo.fedId] > 0 ) return false;
  
  const uint32_t fedSize = fedInfo.fedSize();
  fedSizes[fedInfo.fedId] = fedSize;
  eventSize += fedSize;

  updatePaddingSize();
  
  return true;
}


inline
void evb::bu::Event::EventInfo::updatePaddingSize()
{
  const size_t pageSize = sysconf(_SC_PAGE_SIZE);
  paddingSize = pageSize - (headerSize + eventSize) % pageSize;
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
