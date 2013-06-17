#ifndef _evb_FragmentTracker_h_
#define _evb_FragmentTracker_h_

#include <ostream>
#include <stdint.h>

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
      const uint32_t fedSizeStdDev
    );
    
    ~FragmentTracker();

    /**
     * Starts a new FED fragment with the specified event number.
     * Return the size of the FED data.
     */
    uint32_t startFragment(const uint32_t eventNumber);

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
    
    uint32_t getFedSize();
    
    enum FedComponent
    {
      FED_HEADER,
      FED_PAYLOAD,
      FED_TRAILER
    };
    
    const uint32_t fedId_;
    const uint32_t fedSize_;
    toolbox::math::LogNormalGen* logNormalGen_;
    uint16_t fedCRC_;
    FedComponent typeOfNextComponent_;
    uint32_t currentFedSize_;
    uint32_t remainingFedSize_;
    uint32_t eventNumber_;
  };
    
} // namespace evb

#endif // _evb_FragmentTracker_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
