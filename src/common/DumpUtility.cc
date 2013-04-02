#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "evb/DumpUtility.h"


void evb::DumpUtility::dump
(
  std::ostream& s,
  toolbox::mem::Reference* head
) 
{
  toolbox::mem::Reference *bufRef = 0;
  uint32_t bufferCnt = 0;
  
  s << "\n==================== DUMP ======================\n";
  
  for (
    bufRef=head, bufferCnt=1;
    bufRef != 0;
    bufRef=bufRef->getNextReference(), bufferCnt++
  ) 
  {
    dumpBlock(s, bufRef, bufferCnt);
  }
  
  s << "================ END OF DUMP ===================\n";
}


void evb::DumpUtility::dumpBlock
(
  std::ostream& s,
  toolbox::mem::Reference* bufRef,
  const uint32_t bufferCnt
)
{
  const uint32_t dataSize = bufRef->getDataSize();
  const unsigned char* data = (unsigned char*)bufRef->getDataLocation();
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)data;
  const unsigned char* payloadPointer = data +
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  
  s << "Buffer counter       (dec): ";
  s << bufferCnt << "\n";
  s << "Buffer data location (hex): ";
  s << toolbox::toString("%x", data) << "\n";
  s << "Buffer data size     (dec): ";
  s << dataSize << "\n";
  s << "Pointer to payload   (hex): ";
  s << toolbox::toString("%x", payloadPointer) << "\n";
  s << "totalBlocks          (dec): ";
  s << (uint32_t)(block->nbBlocksInSuperFragment) << "\n";
  s << "CurrentBlock         (dec): ";
  s << (uint32_t)(block->blockNb) << "\n";
  s << "Trigger              (dec): ";
  s << (uint32_t)(block->eventNumber) << "\n";
  s << "Resync count         (dec): ";
  s << toolbox::toString("%x", (uint32_t)(block->resyncCount)) << "\n";
  
  dumpBlockData(s, data, dataSize);
}


void evb::DumpUtility::dumpBlockData
(
  std::ostream& s,
  const unsigned char* data,
  uint32_t len
)
{
  uint32_t* d = (uint32_t*)data;
  len /= 4;
  
  for (uint32_t ic=0; ic<len; ic=ic+4) 
  {
    // avoid to write beyond the buffer:
    if (ic + 2 >= len) 
    {
      s << toolbox::toString("%04d %08x %08x", ic*4, d[ic+1], d[ic]);
      s << "\n";
    } 
    else 
    {
      s << toolbox::toString("%04d %08x %08x %08x %08x", 
        ic*4, d[ic+1], d[ic], d[ic+3], d[ic+2]);
      s << "\n";
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
