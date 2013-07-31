#ifndef _evb_Constants_h_
#define _evb_Constants_h_

#include <stdint.h>
#include <string>


namespace evb {

const uint32_t DEFAULT_NB_EVENTS              =  8192;
const uint32_t DEFAULT_MESSAGE_AGE_LIMIT_MSEC =  1000;
const uint16_t TRIGGER_BITS_COUNT             =    64;
const unsigned int GTP_FED_ID                 =   812; //0x32c
const uint16_t FED_COUNT                      =  1024;
const uint16_t FEROL_BLOCK_SIZE               =  4096;

const std::string HYPERDAQ_ICON = "/hyperdaq/images/HyperDAQ.jpg";

} // namespace evb

#endif
