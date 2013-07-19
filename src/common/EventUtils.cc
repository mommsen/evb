#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/frl_header.h"
#include "evb/CRC16.h"
#include "evb/EventUtils.h"
#include "evb/Exception.h"

#include <algorithm>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <stdlib.h>

evb::L1Information::L1Information()
{
  reset();
}


void evb::L1Information::reset()
{
  strncpy(reason, "none", sizeof(reason));
  isValid           = false;
  runNumber         = 0UL;
  lsNumber          = 0UL;
  bunchCrossing     = 0UL;
  orbitNumber       = 0UL;
  eventType         = 0U;
  l1Technical       = 0ULL;
  l1Decision_0_63   = 0ULL;
  l1Decision_64_127 = 0ULL;
}


const evb::L1Information& evb::L1Information::operator=
(
  const L1Information& l1Info
)
{
  strncpy(reason, l1Info.reason, sizeof(reason));
  isValid           = l1Info.isValid;
  runNumber         = l1Info.runNumber;
  lsNumber          = l1Info.lsNumber;
  bunchCrossing     = l1Info.bunchCrossing;
  orbitNumber       = l1Info.orbitNumber;
  eventType         = l1Info.eventType;
  l1Technical       = l1Info.l1Technical;
  l1Decision_0_63   = l1Info.l1Decision_0_63;
  l1Decision_64_127 = l1Info.l1Decision_64_127;
  return *this;
}


void evb::setFakeTriggerBits
(
  int32_t patternScheme,
  L1Information& l1Info
)
{
  l1Info.isValid = true;

  //static const uint64_t bitPattern0   = 0ULL;
  static const uint64_t bitPattern1   = 0x5555555555555555ULL;
  static const uint64_t bitPattern2   = 0xaaaaaaaaaaaaaaaaULL;
  static const uint64_t bitPatternf   = 0xffffffffffffffffULL;

  switch(patternScheme)
  {
    case -1:
      l1Info.l1Technical       = (l1Info.lsNumber % 2) ? bitPattern1 : bitPattern2;
      l1Info.l1Decision_0_63   = (l1Info.lsNumber % 2) ? bitPattern2 : bitPattern1;
      l1Info.l1Decision_64_127 = (l1Info.lsNumber % 2) ? bitPattern1 : bitPattern2;
      break;
    case -2:
      l1Info.l1Technical       = bitPatternf;
      l1Info.l1Decision_0_63   = bitPatternf;
      l1Info.l1Decision_64_127 = bitPatternf;
      break;
    case 256:
      break;
    default:
      switch(l1Info.lsNumber % 3)
      {
        case 0: l1Info.l1Decision_0_63   |= (1ULL << patternScheme); break;
        case 1: l1Info.l1Decision_64_127 |= (1ULL << patternScheme); break;
        case 2: l1Info.l1Technical       |= (1ULL << patternScheme); break;
      }
  }
}


uint32_t evb::checkFrlHeader
(
  toolbox::mem::Reference* bufRef
)
{
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  const void* payload = (void*)((unsigned char*)block +
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME));
  const frlh_t* frlHeader = (frlh_t*)payload;

  if (frlHeader->trigno != block->eventNumber)
  {
    std::stringstream oss;

    oss << "Event number of RU builder header does not match that of FRL header";
    oss << " block->eventNumber: " << block->eventNumber;
    oss << " frlHeader->trigno: " << frlHeader->trigno;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  if (frlHeader->segno != block->blockNb)
  {
    std::stringstream oss;

    oss << "Block number of RU builder header does not match that of FRL header";
    oss << " block->blockNb: " << block->blockNb;
    oss << " frlHeader->segno: " << frlHeader->segno;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  const size_t expectedSegSize = bufRef->getDataSize()
    - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME)
    - sizeof(frlh_t);

  const uint32_t segSize = frlHeader->segsize & FRL_SEGSIZE_MASK;

  if (segSize != expectedSegSize)
  {
    std::stringstream oss;

    oss << "FRL header segment size is not as expected.";
    oss << " Expected: " << expectedSegSize;
    oss << " Received: " << segSize;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  // Check that RUbuilder and FRL headers agree on end of super-fragment
  if (
    (block->blockNb == (block->nbBlocksInSuperFragment - 1)) !=
    ((frlHeader->segsize & FRL_LAST_SEGM) != 0)
  )
  {
    std::stringstream oss;

    oss << "End of super-fragment of FU header does not match FRL header.";
    oss << " FU header: ";
    oss << (block->blockNb == (block->nbBlocksInSuperFragment - 1));
    oss << " FRL header: ";
    oss << ((frlHeader->segsize & (~FRL_LAST_SEGM)) != 0);

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  return segSize;
}


void evb::checkFedHeader
(
  toolbox::mem::Reference* bufRef,
  const uint32_t offset,
  FedInfo& fedInfo
)
{
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  const void* payload = (void*)((unsigned char*)block +
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) + sizeof(frlh_t) + offset);
  const fedh_t* fedHeader = (fedh_t*)payload;

  if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
  {
    std::stringstream oss;

    oss << "Expected FED header of event " << block->eventNumber;
    oss << " but got event id 0x" << std::hex << fedHeader->eventid;
    oss << " and source id 0x" << std::hex << fedHeader->sourceid;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  const uint32_t eventid = FED_LVL1_EXTRACT(fedHeader->eventid);
  if (eventid != block->eventNumber)
  {
    std::stringstream oss;

    oss << "FED header \"eventid\" does not match";
    oss << " RU builder header \"eventNumber\"";
    oss << " eventid: " << eventid;
    oss << " eventNumber: " << block->eventNumber;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  fedInfo.fedId = FED_SOID_EXTRACT(fedHeader->sourceid);

  if ( fedInfo.fedId >= FED_COUNT )
  {
    std::stringstream oss;

    oss << "The FED id " << fedInfo.fedId << " is larger than the maximum " << FED_COUNT;
    oss << " in event " << block->eventNumber;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  checkCRC(fedInfo, block->eventNumber);
}


void evb::checkFedTrailer
(
  toolbox::mem::Reference* bufRef,
  const uint32_t segSize,
  FedInfo& fedInfo
)
{
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  fedInfo.trailer = (fedt_t*)((unsigned char*)block +
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
    sizeof(frlh_t) + segSize - sizeof(fedt_t));

  if ( FED_TCTRLID_EXTRACT(fedInfo.trailer->eventsize) != FED_SLINK_END_MARKER )
  {
    std::stringstream oss;

    oss << "Expected FED trailer of event " << block->eventNumber;
    oss << " but got event size 0x" << std::hex << fedInfo.trailer->eventsize;
    oss << " and conscheck 0x" << std::hex << fedInfo.trailer->conscheck;

    XCEPT_RAISE(exception::SuperFragment, oss.str());
  }

  // Force CRC field to zero before re-computing the CRC.
  // See http://people.web.psi.ch/kotlinski/CMS/Manuals/DAQ_IF_guide.html
  fedInfo.conscheck = fedInfo.trailer->conscheck;
  fedInfo.trailer->conscheck = 0;
}


inline void evb::checkCRC
(
  FedInfo& fedInfo,
  const uint32_t eventNumber
)
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


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
