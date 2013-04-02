#include <algorithm>
#include <stdlib.h>
#include <sys/mman.h>
#include <sstream>

//#include <boost/crc.hpp> 

#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/FUproxy.h"
#include "evb/bu/FuRqstForResource.h"
#include "evb/CRC16.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "xcept/tools.h"


evb::bu::Event::Event
(
  const uint32_t ruCount,
  toolbox::mem::Reference* bufRef
) :
offset_(0),
nbExpectedSuperFragments_(ruCount+1), // RUs + 1 EVM
nbCompleteSuperFragments_(1)
{  
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  buResourceId_ = block->buResourceId;
  evbId_ = EvBid(block->resyncCount, block->eventNumber);
  eventInfo_ = new EventInfo(block->runNumber, block->lumiSection, block->eventNumber);
  
  payload_ = (((I2O_MESSAGE_FRAME*)block)->MessageSize << 2) -
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  
  SuperFragmentDescriptorPtr superFragment(
    new SuperFragmentDescriptor(bufRef)
  );
  data_.insert(Data::value_type(0, superFragment));
}


evb::bu::Event::~Event()
{
  delete eventInfo_;
  data_.clear();
  fedLocations_.clear();
}


void evb::bu::Event::appendSuperFragment
(
  toolbox::mem::Reference* bufRef
)
{
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  payload_ += (((I2O_MESSAGE_FRAME*)block)->MessageSize << 2) -
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

  // Only keep non-empty super fragments
  if ( bufRef->getDataSize() > sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) )
  {
    Data::iterator pos = data_.lower_bound(block->superFragmentNb);
    if ( pos == data_.end() || (data_.key_comp()(block->superFragmentNb, pos->first)) )
    {
      // Use pos as hint to insert new super fragment
      SuperFragmentDescriptorPtr superFragment(
        new SuperFragmentDescriptor(bufRef)
      );
      data_.insert(pos, Data::value_type(block->superFragmentNb, superFragment));
    }
    else
    {
      pos->second->append(bufRef);
    }
  }
  
  // If the event data block is the last of its super-fragment
  if ( block->blockNb == (block->nbBlocksInSuperFragment-1) )
  {
    // Increment the count of completed super-fragments
    ++nbCompleteSuperFragments_;
  }
}


bool evb::bu::Event::isComplete() const
{
  return ( nbExpectedSuperFragments_ == nbCompleteSuperFragments_ );
}

void evb::bu::Event::parseAndCheckData()
{
  if ( ! isComplete() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot check an incomplete event for data integrity.");
  }

  Data::const_iterator it = data_.begin();
  const Data::const_iterator itEnd = data_.end();
  toolbox::mem::Reference* bufRef = it->second->get();
  
  try
  {
    checkTriggerFragment(bufRef);
  }
  catch(exception::SuperFragment &e)
  {
    const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
        (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
    std::ostringstream oss;
    
    oss << "Received a bad trigger fragment.";
    oss << " eventNumber: " << block->eventNumber;
    oss << " resyncCount: " << block->resyncCount;
    oss << " buResourceId: " << block->buResourceId;
    oss << " superFragmentNb: " << block->superFragmentNb;
    DumpUtility::dump(oss, bufRef);
    
    XCEPT_RETHROW(exception::SuperFragment, oss.str(), e);
  }
  
  while ( ++it != itEnd )
  {
    bufRef = it->second->get();
  
    try
    {
      checkSuperFragment(bufRef);
    }
    catch(exception::SuperFragment &e)
    {
      const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
        (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
      std::ostringstream oss;
      
      oss << "Received a bad super fragment.";
      oss << " eventNumber: " << block->eventNumber;
      oss << " resyncCount: " << block->resyncCount;
      oss << " buResourceId: " << block->buResourceId;
      oss << " superFragmentNb: " << block->superFragmentNb;
      DumpUtility::dump(oss, bufRef);
      
      XCEPT_RETHROW(exception::SuperFragment, oss.str(), e);
    }
  }
}


void evb::bu::Event::sendToFU
(
  boost::shared_ptr<FUproxy> fuProxy,
  const FuRqstForResource& rqst
) const
{
  if ( ! isComplete() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot send an incomplete event to a FU.");
  }

  uint32_t superFragmentNb = 0;

  for (
    Data::const_iterator it = data_.begin(), itEnd = data_.end();
    it != itEnd; ++it
  )
  {
    // Send a duplicate of the reference so memory is freed on FU discard
    fuProxy->sendSuperFragment(
      rqst, superFragmentNb, nbCompleteSuperFragments_,
      it->second->duplicate()
    );
    
    ++superFragmentNb;
  }
}


void evb::bu::Event::writeToDisk(FileHandlerPtr fileHandler)
{
  if ( fedLocations_.empty() )
  {
    XCEPT_RAISE(exception::EventOrder, "Cannot find any FED data. Has the event been parsed?");
  }
  
  // Get the memory mapped file chunk
  const size_t bufferSize = eventInfo_->getBufferSize();
  char* map = (char*)fileHandler->getMemMap(bufferSize);

  // Write event information
  memcpy(map, eventInfo_, eventInfo_->headerSize);

  // Write event
  const size_t locations = fedLocations_.size();
  char* filepos = map+eventInfo_->headerSize;

  for (size_t i=locations; i > 0 ; --i)
  {
    const FedLocationPtr loc = fedLocations_[i-1];
    memcpy(filepos+loc->offset, loc->location, loc->length);
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


void evb::bu::Event::checkTriggerFragment(toolbox::mem::Reference* bufRef)
{
  const size_t bufSize = bufRef->getDataSize();
  const size_t minimumBufSize =
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
    sizeof(frlh_t) + sizeof(fedh_t) + sizeof(fedt_t);
  
  if (bufSize < minimumBufSize)
  {
    std::stringstream oss;
    
    oss << "Trigger message is too small.";
    oss << " Minimum size: " << minimumBufSize;
    oss << " Received: "     << bufSize;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();

  if (block->blockNb != 0)
  {
    std::stringstream oss;
    
    oss << "Trigger block number is not 0.";
    oss << " Received: " << block->blockNb;
    
    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }
  
  checkSuperFragment(bufRef);
}


void evb::bu::Event::checkSuperFragment(toolbox::mem::Reference* head)
{
  typedef std::vector<toolbox::mem::Reference*> Chain;
  Chain chain;

  toolbox::mem::Reference* bufRef = head;
  while (bufRef)
  {
    chain.push_back(bufRef);
    bufRef = bufRef->getNextReference();
  }

  Chain::const_reverse_iterator rit = chain.rbegin();
  const Chain::const_reverse_iterator ritEnd = chain.rend();

  uint32_t segSize = 0;
  
  do {
    if (segSize == 0) segSize = checkFrlHeader(*rit);
    FedInfo fedInfo;
    checkFedTrailer(*rit,segSize,fedInfo);
    //const size_t lastFragment = fedLocations_.size() - 1;

    uint32_t remainingFedSize = fedInfo.fedSize();
    offset_ += remainingFedSize;
    size_t fedOffset = offset_;

    // now start the odyssee to the fragment header
    // advance to the block where we expect the header
    while ( remainingFedSize > segSize ) 
    {
      fedOffset -= segSize;
      FedLocationPtr fedLocation = FedLocationPtr( new FedLocation(
        (unsigned char*)((toolbox::mem::Reference*)(*rit)->getDataLocation()) +
        sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
        sizeof(frlh_t),
        segSize, fedOffset));
      fedLocations_.push_back(fedLocation);

      // advance to the next block
      remainingFedSize -= segSize;
      ++rit;
      if ( rit == chain.rend() ) 
      {
        XCEPT_RAISE(exception::SuperFragment,"Corrupted superfragment: Premature end of block chain encountered.");
      }
      
      segSize = checkFrlHeader(*rit);
    }
    
    // segSize now points to the end of the previous fragment or is 0
    segSize -= remainingFedSize;
    fedOffset -= remainingFedSize;
    FedLocationPtr fedLocation = FedLocationPtr( new FedLocation(
      (unsigned char*)((toolbox::mem::Reference*)(*rit)->getDataLocation()) +
      sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
      sizeof(frlh_t) +
      segSize,
      remainingFedSize, fedOffset));
    fedLocations_.push_back(fedLocation);

    //fedInfo.crc = updateCRC(fedLocations_.size()-1, lastFragment);

    // the header must be in the current block
    checkFedHeader(*rit, segSize, fedInfo);
    if ( !eventInfo_->addFedSize(fedInfo) )
    {
      const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
        (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)
        ((toolbox::mem::Reference*)(*rit))->getDataLocation();
      std::stringstream oss;
      
      oss << "Duplicated FED id " << fedInfo.fedId;
      oss << " found in event " << block->eventNumber;
      
      XCEPT_RAISE(exception::SuperFragment, oss.str());
    }
    
    if ( segSize == 0 ) ++rit; // the next trailer is in a new block
    
  } while ( rit != ritEnd );
}


uint16_t evb::bu::Event::updateCRC
(
  const size_t& first,
  const size_t& last
)
{
  //boost::crc_optimal<16, 0x8005, 0xFFFF, 0, false, false> crc;
  uint16_t crc(0xFFFF);

  for (size_t i = first; i != last; --i)
  {
    const FedLocationPtr loc = fedLocations_[i];
    const size_t wordCount = loc->length/8;
    for (size_t w=0; w<wordCount; ++w)
    {
      //crc.process_block(&loc->location[w*8+7],&loc->location[w*8]);
      for (int b=7; b >= 0; --b)
      {
        const unsigned char index = (crc >> 8) ^ loc->location[w*8+b];
        crc <<= 8;
        crc ^= evf::crc_table[index];
      }
    }
  }
  
  return crc; //.checksum();
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
