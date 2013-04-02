#ifndef _evb_bu_Event_h_
#define _evb_bu_Event_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/FileHandler.h"
#include "evb/bu/SuperFragmentDescriptor.h"
#include "evb/EvBid.h"
#include "evb/EventUtils.h"
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
        const uint32_t ruCount,
        toolbox::mem::Reference*
      );
      
      ~Event();
      
      /**
       * Append a super fragment to the event
       */
      void appendSuperFragment(toolbox::mem::Reference*);
      
      /**
       * Return true if all super fragments have been received
       */
      bool isComplete() const;
      
      /**
       * Check the complete event for integrity of the data
       */
      void parseAndCheckData();
      
      /**
       * Write the event to disk using the handler passed
       */
      void writeToDisk(FileHandlerPtr);
      
      /**
       * Send the event to the FU specified in the request.
       */
      void sendToFU
      (
        boost::shared_ptr<FUproxy> fuProxy,
        const FuRqstForResource& rqst
      ) const;
      
      /**
       * Return the buResourceId
       */
      uint32_t buResourceId() const
      { return buResourceId_; }
      
      /**
       * Return the evb id
       */
      EvBid evbId() const
      { return evbId_; }
      
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
       * Return the payload in Bytes
       */
      size_t payload() const
      { return payload_; }
      
      
    private:
      
      // Map to hold super fragments index by RU instance
      typedef std::map<uint32_t,SuperFragmentDescriptorPtr> Data;
      Data data_;
      boost::mutex mutex_;
      
      struct EventInfo
      {
        const uint32_t version;
        const uint32_t runNumber;
        const uint32_t lumiSection;
        const uint32_t eventNumber;
        uint32_t eventSize;
        uint32_t paddingSize;
        uint32_t fedSizes[FED_COUNT];
        
        static const size_t headerSize = sizeof(uint32_t)*(6+FED_COUNT);
        
        EventInfo(
          const uint32_t runNumber,
          const uint32_t lumiSection,
          const uint32_t eventNumber
        );
        
        bool addFedSize(const FedInfo&);
        void updatePaddingSize();
        inline size_t getBufferSize()
        { return ( headerSize + eventSize + paddingSize ); }
      };
      EventInfo* eventInfo_;
      
      size_t offset_;
      struct FedLocation
      {
        const unsigned char* location;
        const uint32_t length;
        const size_t offset;
        
        FedLocation(const unsigned char* loc, const uint32_t len, const size_t offset) :
        location(loc),length(len),offset(offset) {};
      };
      typedef boost::shared_ptr<FedLocation> FedLocationPtr;
      typedef std::vector<FedLocationPtr> FedLocations;
      FedLocations fedLocations_;
      
      void checkTriggerFragment(toolbox::mem::Reference*);
      void checkSuperFragment(toolbox::mem::Reference*);
      uint16_t updateCRC
      (
        const size_t& first,
        const size_t& last
      );
      
      const uint32_t nbExpectedSuperFragments_;
      uint32_t nbCompleteSuperFragments_;
      
      uint32_t buResourceId_;
      EvBid evbId_;
      uint32_t eventNumber_;
      size_t payload_;
      
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
