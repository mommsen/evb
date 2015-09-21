#ifndef _evb_bu_Event_h_
#define _evb_bu_Event_h_

#include <boost/shared_ptr.hpp>

#include <limits>
#include <map>
#include <stdint.h>
#include <vector>

#include "evb/bu/EventInfo.h"
#include "evb/bu/FedInfo.h"
#include "evb/DataLocations.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "i2o/i2oDdmLib.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  namespace bu {

    class FuRqstForResource;
    class FUproxy;

    /**
     * \ingroup xdaqApps
     * \brief Represent an event
     */

    class Event
    {
    public:

      Event
      (
        const EvBid&,
        const msg::RUtids&,
        const uint16_t buResourceId,
        const bool checkCRC,
        const bool calculateCRC32
      );

      ~Event();

      /**
       * Append a super fragment to the event.
       * Return true if this completes the event.
       */
      bool appendSuperFragment
      (
        const I2O_TID ruTid,
        toolbox::mem::Reference*,
        unsigned char* payload
      );

      /**
       * Return true if all super fragments have been received
       */
      bool isComplete() const
      { return ruSizes_.empty(); }

      /**
       * Check the complete event for integrity of the data
       */
      void checkEvent() const;

      /**
       * Write the event as binary dump to a text file
       */
      void dumpEventToFile(const std::string& reasonForDump, const uint32_t badChunk=std::numeric_limits<uint32_t>::max()) const;

      EvBid getEvBid() const { return evbId_; }
      uint16_t buResourceId() const { return buResourceId_; }
      EventInfoPtr getEventInfo() const { return eventInfo_; }
      DataLocations getDataLocations() const { return dataLocations_; }

    private:

      DataLocations dataLocations_;

      EventInfoPtr eventInfo_;

      typedef std::vector<toolbox::mem::Reference*> BufferReferences;
      BufferReferences myBufRefs_;

      const EvBid evbId_;
      const bool checkCRC_;
      const bool calculateCRC32_;
      const uint16_t buResourceId_;
      typedef std::map<I2O_TID,uint32_t> RUsizes;
      RUsizes ruSizes_;

      msg::FedIds missingFedIds_;

    }; // Event

    typedef boost::shared_ptr<Event> EventPtr;

  } } // namespace evb::bu

#endif // _evb_bu_Event_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
