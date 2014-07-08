#ifndef _evb_Constants_h_
#define _evb_Constants_h_

#include <stdint.h>


namespace evb {

  //const uint16_t GTP_FED_ID          =     812; //0x32c
  const uint16_t GTP_FED_ID          =    1055; //temporary hack until TCDS is fully operational
  const uint16_t GTPe_FED_ID         =     814; //0x32e
  const uint16_t TCDS_FED_ID         =    1024; // to be defined
  const uint32_t ORBITS_PER_LS       = 0x40000; //2^18 orbits
  const uint16_t FED_COUNT           =    1350;
  const uint16_t FEROL_BLOCK_SIZE    =    4096;

} // namespace evb

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
