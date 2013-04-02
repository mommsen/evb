#ifndef _evb_SuperFragmentTracker_h_
#define _evb_SuperFragmentTracker_h_

#include <ostream>

#include "toolbox/math/random.h"


namespace evb {

  class SuperFragmentTracker
  {
  public:
    
    /**
     * Constructor.
     */
    SuperFragmentTracker
    (
      const uint32_t fedPayloadSize,
      const uint32_t fedPayloadStdDev
    );

    /**
     * The types of the components of FED data.
     */
    enum FedComponent
    {
      FED_HEADER,
      FED_PAYLOAD,
      FED_TRAILER
    };
    
    /**
     * Gives the size and type a component of FED data.
     */
    struct FedComponentDescriptor
    {
      /**
       * The size of the FED data component in bytes.
       */
      size_t size;
      
      /**
       * The type of the FED data component.
       */
      FedComponent type;

      /**
       * The FED id
       */
      unsigned int fedId;
    };

    typedef std::vector<uint32_t> FedSourceIds;
    
    /**
     * Starts a new super-fragment with the specified dimensions.
     */
    void startSuperFragment
    (
      const FedSourceIds& fedSourceIds
    );

    /**
     * Returns true if the end of the super-fragment has been reached.
     */
    bool reachedEndOfSuperFragment() const;

    /**
     * Returns the size and type of the next component.  If the component
     * cannot fit within the specified number of bytes available, then the
     * minimum size of the component is returned.
     */
    FedComponentDescriptor getNextComponent
    (
      const size_t nbBytesAvailable
    ) const;

    /**
     * Moves the tracker to the next component.
     */
    void moveToNextComponent
    (
      const size_t nbBytesAvailable
    );

    /**
     * Return the size of the current FED
     */
    uint32_t getFedPayloadSize() const
    { return fedPayloadSize_; }

private:

    uint32_t fedPayloadSize_;
    const bool useLogNormal_;
    toolbox::math::LogNormalGen logNormalGen_;
    
    FedSourceIds fedSourceIds_;
    FedSourceIds::const_iterator currentFed_;
    FedComponent typeOfNextComponent_;
    uint32_t remainingFedPayload_;
  };
    
} // namespace evb


/**
 * Prints the specified type of FED data component.
 */
inline std::ostream& operator<<
(
  std::ostream& s,
  evb::SuperFragmentTracker::FedComponent& c
)
{
  switch(c)
  {
    case evb::SuperFragmentTracker::FED_HEADER:
      s << "FED_HEADER";
      break;
    case evb::SuperFragmentTracker::FED_PAYLOAD:
      s << "FED_PAYLOAD";
      break;
    case evb::SuperFragmentTracker::FED_TRAILER:
      s << "FED_TRAILER";
      break;
    default:
      s << "UNKNOWN";
  }
  
  return s;
}


#endif // _evb_SuperFragmentTracker_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
