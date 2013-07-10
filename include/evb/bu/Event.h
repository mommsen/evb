#ifndef _evb_bu_Event_h_
#define _evb_bu_Event_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/FileHandler.h"
#include "evb/EvBid.h"
#include "evb/EventUtils.h"
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
        const unsigned char* fragmentPos,
        const uint32_t partSize,
        const uint32_t totalSize
      );
      
      /**
       * Return true if all super fragments have been received
       */
      bool isComplete() const
      { return ruSizes_.empty(); }
      
      /**
       * Check the complete event for integrity of the data
       */
      void checkEvent();
      
      /**
       * Write the event to disk using the handler passed
       */
      void writeToDisk(FileHandlerPtr);
      
      /**
       * Return the event-builder id of the event
       */
      EvBid getEvBid() const
      { return evbId_; }
      
      /**
       * Return the resource id
       */
      uint32_t buResourceId() const
      { return buResourceId_; }
      
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
      
      struct EventInfo
      {
        const uint32_t version;
        const uint32_t runNumber;
        const uint32_t eventNumber;
        const uint32_t lumiSection;
        uint32_t eventSize;
        uint32_t paddingSize;
        uint32_t fedSizes[FED_COUNT];
        
        static const size_t headerSize = sizeof(uint32_t)*(6+FED_COUNT);
        
        EventInfo(
          const uint32_t runNumber,
          const uint32_t eventNumber,
          const uint32_t lumiSection
        );
        
        bool addFedSize(const FedInfo&);
        void updatePaddingSize();
        inline size_t getBufferSize()
        { return ( headerSize + eventSize + paddingSize ); }
      };
      EventInfo* eventInfo_;
      
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
      typedef std::vector<toolbox::mem::Reference*> BufferReferences;
      BufferReferences myBufRefs_;
      
      void checkFedHeader
      (
        const unsigned char* pos,
        const uint32_t offset,
        FedInfo&
      ) const;
      void checkFedTrailer
      (
        const unsigned char* pos,
        const uint32_t offset,
        FedInfo&
      ) const;
      void checkCRC
      (
        FedInfo&,
        const uint32_t eventNumber
      ) const;
      uint16_t updateCRC
      (
        const size_t& first,
        const size_t& last
      ) const;
      
      const EvBid evbId_;
      const uint32_t buResourceId_;
      typedef std::map<I2O_TID,uint32_t> RUsizes;
      RUsizes ruSizes_;
      
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
