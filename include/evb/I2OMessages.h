#ifndef _evb_I2OMessages_h_
#define _evb_I2OMessages_h_

#include <iostream>
#include <stdint.h>
#include <vector>

#include "i2o/i2o.h"
#include "i2o/i2oDdmLib.h"
#include "evb/EvBid.h"


namespace evb {
  namespace msg {

    using EvBids = std::vector<EvBid>;
    using RUtids = std::vector<I2O_TID>;
    using FedIds = std::vector<uint16_t>;

    /**
     * Event request for one or more events identified by the event-builder ids to be sent
     * to the BU TID specified. The EVM ignores the evbIds and sends the next nbRequests
     * fragements available. The RU TIDs are filled by the EVM.
     */
    struct EventRequest
    {
      I2O_TID buTid;                             // BU TID to send the data
      uint32_t msgSize;                          // Size of the message
      uint16_t padding;
      uint16_t priority;                         // Priority of the request (0 is highest)
      uint64_t timeStampNS;                      // time stamp in ns set by the BU when sending request
      uint16_t buResourceId;                     // Index of BU resource used to built the event
      uint16_t nbRequests;                       // Number of requested EvBids
      uint16_t nbDiscards;                       // Number of previously sent EvBids to discard
      uint16_t nbRUtids;                         // Number of RU TIDs
      EvBid evbIds[];                            // EvBids
      I2O_TID ruTids[];                          // List of RU TIDs participating in the event building

      void getEvBids(EvBids&) const;
      void getRUtids(RUtids&) const;
    };


    /**
     * BU to EVM to RU message that contains one or more event requests
     */
    struct ReadoutMsg
    {
      I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information
      uint32_t padding;
      uint32_t nbRequests;                       // Number of requests
      EventRequest requests[];                   // List of event requests
    };


    /**
     * A super-fragment containing data from all FEDs connected to the RU
     */
    struct SuperFragment
    {
      uint16_t headerSize;                       // Size of the message header
      uint16_t superFragmentNb;                  // Index of the super fragment
      uint32_t totalSize;                        // Total size of the super fragment
      uint32_t partSize;                         // Partial size of the super-fragment contained in this message
      uint16_t nbDroppedFeds;                    // Number of FEDs dropped from the super fragment
      uint16_t fedIds[];                         // List of dropped FED ids

      void appendFedIds(FedIds&) const;

    };


    /**
     * EVM/RU to BU message that contains one or more super-fragments
     */
    struct I2O_DATA_BLOCK_MESSAGE_FRAME
    {
      I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information
      uint16_t headerSize;                       // Size of the message header
      uint16_t buResourceId;                     // Index of BU resource used to built the event
      uint32_t nbBlocks;                         // Total number of I2O blocks
      uint32_t blockNb;                          // Index of the this block
      uint64_t timeStampNS;                      // time stamp in ns set by the BU when sending request
      uint16_t nbSuperFragments;                 // Total number of super fragments
      uint16_t nbRUtids;                         // Number of RU TIDs
      EvBid evbIds[];                            // The EvBids of the super fragments
      I2O_TID ruTids[];                          // List of RU TIDs participating in the event building

      void getEvBids(EvBids&) const;
      void getRUtids(RUtids&) const;

    };

    std::ostream& operator<<
    (
      std::ostream& s,
      const I2O_PRIVATE_MESSAGE_FRAME
    );


    std::ostream& operator<<
    (
      std::ostream&,
      const evb::msg::ReadoutMsg*
    );


    std::ostream& operator<<
    (
      std::ostream&,
      const evb::msg::SuperFragment
    );


    std::ostream& operator<<
    (
      std::ostream&,
      const evb::msg::I2O_DATA_BLOCK_MESSAGE_FRAME&
    );

  } } // namespace evb::msg


#endif // _evb_I2OMessages_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
