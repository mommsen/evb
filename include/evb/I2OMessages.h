#ifndef _evb_I2OMessages_h_
#define _evb_I2OMessages_h_

#include <iostream>
#include <stdint.h>

#include "i2o/i2o.h"
#include "evb/EvBid.h"


namespace evb {
  namespace msg {
        
    /**
     * BU to RU message that contains one or more requests for event
     * fragments for the passed event-builder id. If the first
     * EvBid is not valid, the next available <nbRequests> events are sent.
     */
    typedef struct
    {
      I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information
      uint32_t buResourceId;                     // Index of BU resource used to built the event
      int32_t nbRequests;                        // Number of requests. If negative, ignore evbIds 
      EvBid evbIds[1];                           // Requests
      
    } RqstForFragmentsMsg;
    
    
    /**
     * A super-fragment containing data from all FEDs connected to the RU 
     */
    typedef struct
    {
      EvBid evbId;                               // The EvBid of the event
      uint32_t nbSuperFragments;                 // Total number of super fragments
      uint32_t superFragmentNb;                  // Index of the super fragment
      uint32_t totalSize;                        // Total size of the super fragment
      uint32_t partSize;                         // Partial size of the super-fragment contained in this message
      
    } SuperFragment;
    
    
    /**
     * RU to BU message that contains one or more super-fragments
     */
    typedef struct
    {
      I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information
      uint32_t buResourceId;                     // Index of BU resource used to built the event
      uint32_t nbBlocks;                         // Total number of I2O blocks
      uint32_t blockNb;                          // Index of the this block
      uint32_t padding;
      
    } SuperFragmentsMsg;

    
  } } // namespace evb::msg


std::ostream& operator<<
(
  std::ostream& s,
  const I2O_PRIVATE_MESSAGE_FRAME&
);


std::ostream& operator<<
(
  std::ostream&,
  const evb::msg::RqstForFragmentsMsg*
);


std::ostream& operator<<
(
  std::ostream&,
  const evb::msg::SuperFragment*
);


std::ostream& operator<<
(
  std::ostream&,
  const evb::msg::SuperFragmentsMsg*
);


#endif // _evb_I2OMessages_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
