#include "evb/DumpUtility.h"
#include "interface/shared/i2ogevb2g.h"


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
  const I2O_DATA_READY_MESSAGE_FRAME* block =
    (I2O_DATA_READY_MESSAGE_FRAME*)data;
  const unsigned char* payloadPointer = data +
    sizeof(I2O_DATA_READY_MESSAGE_FRAME);

  s << "Buffer counter       (dec): ";
  s << bufferCnt << std::endl;
  s << "Buffer data location (hex): ";
  s << toolbox::toString("%x", data) << std::endl;
  s << "Buffer data size     (dec): ";
  s << dataSize << std::endl;
  s << "Pointer to payload   (hex): ";
  s << toolbox::toString("%x", payloadPointer) << std::endl;
  s << "Total length         (dec): ";
  s << block->totalLength << std::endl;
  s << "Partial length       (dec): ";
  s << block->partLength << std::endl;
  s << "FED id               (dec): ";
  s << block->fedid << std::endl;
  s << "Trigger no           (dec): ";
  s << block->triggerno << std::endl;
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
      s << toolbox::toString("%08x : %08x %08x %12s         |    human readable swapped : %08x %08x %12s      : %08x", ic*4, d[ic], d[ic+1], "", d[ic+1], d[ic], "", ic*4);
      s << std::endl;
    }
    else
    {
      s << toolbox::toString("%08x : %08x %08x %08x %08x    |    human readable swapped : %08x %08x %08x %08x : %08x", ic*4, d[ic], d[ic+1], d[ic+2], d[ic+3],  d[ic+1], d[ic], d[ic+3], d[ic+2], ic*4);
      s << std::endl;
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
