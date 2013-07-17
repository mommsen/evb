#ifndef _evb_readoutunit_FragmentRequest_h_
#define _evb_readoutunit_FragmentRequest_h_

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "i2o/i2oDdmLib.h"


namespace evb {

  namespace readoutunit {

    struct FragmentRequest
    {
      I2O_TID  buTid;
      uint32_t buResourceId;
      uint32_t nbRequests;
      std::vector<EvBid> evbIds;
      std::vector<I2O_TID> ruTids;
    };

    typedef boost::shared_ptr<FragmentRequest> FragmentRequestPtr;

  } } // namespace evb::readoutunit


inline std::ostream& operator<<
(
  std::ostream& str,
  const evb::readoutunit::FragmentRequestPtr request
)
{
  if ( request.get() )
  {
    str << "Fragment request:" << std::endl;

    str << "buTid=" << request->buTid << std::endl;
    str << "buResourceId=" << request->buResourceId << std::endl;
    str << "nbRequests=" << request->nbRequests << std::endl;
    if ( !request->evbIds.empty() )
    {
      str << "evbIds:" << std::endl;
      for (uint32_t i=0; i < request->nbRequests; ++i)
        str << "   [" << i << "]: " << request->evbIds[i] << std::endl;
    }
    if ( !request->ruTids.empty() )
    {
      str << "ruTids:" << std::endl;
      for (uint32_t i=0; i < request->ruTids.size(); ++i)
        str << "   [" << i << "]: " << request->ruTids[i] << std::endl;
    }
  }

  return str;
}


#endif // _evb_readoutunit_FragmentRequest_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
