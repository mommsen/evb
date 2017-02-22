#ifndef _evb_FragmentTracker_h_
#define _evb_FragmentTracker_h_

#include <ostream>
#include <stdint.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/FragmentSize.h"


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
      const uint32_t fedSizeStdDev,
      const uint32_t minFedSize,
      const uint32_t maxFedSize,
      const bool computeCRC
    );

    /**
     * Start the clock for determining the available triggers
     */
    void startRun();

    /**
    */
    void setMaxTriggerRate(const uint32_t rate)
    { maxTriggerRate_ = rate; }

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

    void waitForNextTrigger();

    CRCCalculator crcCalculator_;
    boost::scoped_ptr<FragmentSize> fragmentSize_;

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
    uint32_t maxTriggerRate_;
    const bool computeCRC_;
    uint16_t fedCRC_;
    FedComponent typeOfNextComponent_;
    uint32_t currentFedSize_;
    uint32_t remainingFedSize_;
    EvBid evbId_;
    uint64_t lastTime_;
    uint32_t availableTriggers_;
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
