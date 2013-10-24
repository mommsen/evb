#ifndef _evb_FragmentTracker_h_
#define _evb_FragmentTracker_h_

#include <ostream>
#include <stdint.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "toolbox/math/random.h"


namespace evb {

  class FragmentTracker
  {
  public:

    /**
     * Constructor.
     */
    FragmentTracker
    (
      const uint32_t fedId,
      const uint32_t fedSize,
      const bool useLogNormal,
      const uint32_t fedSizeStdDev,
      const uint32_t minFedSize,
      const uint32_t maxFedSize,
      const bool computeCRC
    );

    /**
     * Starts a new FED fragment with the specified event number.
     * Return the size of the FED data.
     */
    uint32_t startFragment(const EvBid&);

    /**
     * Fill the FED data into the payload using at most
     * nbBytesAvailable Bytes. It returns the number of
     * Bytes actually filled.
     */
    size_t fillData
    (
      unsigned char* payload,
      const size_t nbBytesAvailable
    );


  private:

    uint32_t getFedSize() const;

    CRCCalculator crcCalculator_;

    enum FedComponent
    {
      FED_HEADER,
      FED_PAYLOAD,
      FED_TRAILER
    };

    const uint32_t fedId_;
    const uint32_t fedSize_;
    const uint32_t minFedSize_;
    const uint32_t maxFedSize_;
    const bool computeCRC_;
    boost::scoped_ptr<toolbox::math::LogNormalGen> logNormalGen_;
    uint16_t fedCRC_;
    FedComponent typeOfNextComponent_;
    uint32_t currentFedSize_;
    uint32_t remainingFedSize_;
    EvBid evbId_;
  };

  typedef boost::shared_ptr<FragmentTracker> FragmentTrackerPtr;

} // namespace evb

#endif // _evb_FragmentTracker_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
