#ifndef _evb_bu_Event_h_
#define _evb_bu_Event_h_

#include <boost/shared_ptr.hpp>

#include <limits>
#include <map>
#include <stdint.h>

#include "evb/CRCCalculator.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/bu/FileHandler.h"
#include "i2o/i2oDdmLib.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
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
        const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*
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
      void checkEvent(const bool computeCRC) const;

      /**
       * Write the event to disk using the handler passed
       */
      void writeToDisk(FileHandlerPtr, const bool calculateAdler32) const;

      /**
       * Write the event as binary dump to a text file
       */
      void dumpEventToFile(const std::string& reasonForDump, const uint32_t badChunk=std::numeric_limits<uint32_t>::max()) const;

      /**
       * Return the event-builder id of the event
       */
      EvBid getEvBid() const
      { return evbId_; }

      /**
       * Return the resource id
       */
      uint16_t buResourceId() const
      { return buResourceId_; }

      /**
       * Return the lumiSection
       */
      uint32_t eventNumber() const
      { return eventInfo_->eventNumber; }

      /**
       * Return the lumiSection
       */
      uint32_t lumiSection() const
      { return eventInfo_->lumiSection; }

      /**
       * Return the runNumber
       */
      uint32_t runNumber() const
      { return eventInfo_->runNumber; }

      /**
       * Return the event size
       */
      uint32_t eventSize() const
      { return eventInfo_->eventSize; }


    private:

      struct DataLocation
      {
        const unsigned char* location;
        const uint32_t length;

        DataLocation(const unsigned char* loc, const uint32_t len) :
          location(loc),length(len) {};
      };
      typedef boost::shared_ptr<DataLocation> DataLocationPtr;
      typedef std::vector<DataLocationPtr> DataLocations;
      DataLocations dataLocations_;

      class FedInfo
      {
      public:
        FedInfo(const unsigned char* pos, uint32_t& remainingLength);
        void addDataChunk(const unsigned char* pos, uint32_t& remainingLength);
        void checkData(const uint32_t eventNumber, const bool computeCRC) const;

        bool complete() const { return (remainingFedSize_ == 0); }

        uint32_t eventId()  const { return FED_LVL1_EXTRACT(header()->eventid); }
        uint16_t fedId()    const { return FED_SOID_EXTRACT(header()->sourceid); }
        uint32_t fedSize()  const { return FED_EVSZ_EXTRACT(trailer()->eventsize)<<3; }
        uint16_t crc()      const { return FED_CRCS_EXTRACT(trailer()->conscheck); }

      private:
        fedh_t* header() const;
        fedt_t* trailer() const;

        DataLocations fedData_;
        uint32_t remainingFedSize_;

        CRCCalculator crcCalculator_;
      };

      struct EventInfo
      {
        const uint32_t version;
        const uint32_t runNumber;
        const uint32_t lumiSection;
        const uint32_t eventNumber;
        uint32_t eventSize;
        uint32_t paddingSize;
        uint32_t adler32;

        EventInfo(
          const uint32_t runNumber,
          const uint32_t lumiSection,
          const uint32_t eventNumber
        );

        bool addFedSize(const FedInfo&);
        size_t getBufferSize();
      };
      EventInfo* eventInfo_;
      friend std::ostream& operator<<(std::ostream&, const EventInfo&);

      typedef std::vector<toolbox::mem::Reference*> BufferReferences;
      BufferReferences myBufRefs_;

      const EvBid evbId_;
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
