#ifndef _evb_EventUtils_h_
#define _evb_EventUtils_h_

#include <stdint.h>
#include <vector>

#include "i2o/i2o.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/GlobalEventNumber.h"
#include "evb/Constants.h"
#include "toolbox/mem/Reference.h"

#include "xdaq/Application.h"


namespace evb {

/**
 * struct for lumi section and L1 trigger bit information
 */
struct L1Information
{
  bool isValid;
  char reason[100];
  uint32_t runNumber;
  uint32_t lsNumber;
  uint32_t bunchCrossing;
  uint32_t orbitNumber;
  uint16_t eventType;
  uint64_t l1Technical;
  uint64_t l1Decision_0_63;
  uint64_t l1Decision_64_127;

  L1Information();
  void reset();
  const L1Information& operator=(const L1Information&);
};

struct FedInfo
{
  uint16_t fedId;
  uint16_t crc;
  uint32_t conscheck;
  fedt_t* trailer;

  FedInfo() : fedId(FED_COUNT), crc(0xffff), conscheck(0), trailer(0) {};
  uint32_t fedSize() const { return ( trailer?FED_EVSZ_EXTRACT(trailer->eventsize)<<3:0 ); }
  //fedt_t* trailer() const { return fedSize>0?(fedt_t*)(&(data[0]) + fedSize - sizeof(fedt_t)):0; }
};


/**
 * Sets the L1 trigger bits to fake patterns using the patternScheme.
 * -2=allbits; -1=built-in patterns; [0,191]=set ONE specific bit; 256=do not modify
 */
void setFakeTriggerBits(int32_t patternScheme, L1Information&);

uint32_t checkFrlHeader(toolbox::mem::Reference*);
void checkFedHeader(toolbox::mem::Reference*, const uint32_t offset, FedInfo&);
void checkFedTrailer(toolbox::mem::Reference*, const uint32_t segsize, FedInfo&);
void checkCRC(FedInfo&, const uint32_t eventNumber);

} // namespace evb

#endif // _evb_EventUtils_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
