#ifndef _evb_bu_FuRqstForResource_h_
#define _evb_bu_FuRqstForResource_h_

#include <stdint.h>
#include <ostream>

#include "i2o/i2oDdmLib.h"


namespace evb { namespace bu { // namespace evb::bu

struct FuRqstForResource
{
  I2O_TID fuTid;
  uint32_t fuTransactionId;
  
  FuRqstForResource() :
  fuTid(0),
  fuTransactionId(0)
  {}
};
    
} } // namespace evb::bu

inline std::ostream& operator<<
(
  std::ostream& s,
  evb::bu::FuRqstForResource& rqst
)
{
  s << "Request from FU tid " << rqst.fuTid
    << " with fuTransactionId " << rqst.fuTransactionId;
  return s;
}



#endif // _evb_bu_FuRqstForResource_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
