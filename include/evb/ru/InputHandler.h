#ifndef _evb_ru_InputHandler_h_
#define _evb_ru_InputHandler_h_

#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>
#include <string>

#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/FragmentChain.h"
#include "evb/FragmentTracker.h"
#include "pt/utcp/frl/MemoryCache.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"


namespace evb {
  
  class RU;
  
  namespace ru {
    
    class InputHandler
    {
    public:
      
      InputHandler() {};
      
      virtual ~InputHandler() {};
      
      /**
       * Callback for I2O_DATA_READY messages received from frontend
       */
      virtual void dataReadyCallback(toolbox::mem::Reference*, pt::utcp::frl::MemoryCache*) = 0;

      /**
       * Get the next complete super fragment.
       * If none is available, the method returns false.
       * Otherwise, the FragmentChainPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      virtual bool getNextAvailableSuperFragment(FragmentChainPtr&) = 0;
      
      /**
       * Get the complete super fragment with EvBid.
       * If it is not available or complete, the method returns false.
       * Otherwise, the FragmentChainPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      virtual bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&) = 0;
 
      /**
       * Configure
       */
      struct Configuration
      {
        bool dropInputData;
        uint32_t dummyFedSize;
        uint32_t dummyFedSizeStdDev;
        uint32_t fragmentPoolSize;
        xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;
        uint16_t triggerFedId;
        bool usePlayback;
        std::string playbackDataFile;
      };
      virtual void configure(const Configuration&) {};
      
      /**
       * Start processing messages
       */
      virtual void startProcessing(const uint32_t runNumber) {};

      /**
       * Remove all data
       */
      virtual void clear() {};
      
    };
    
    class FEROLproxy : public InputHandler
    {
    public:
      
      FEROLproxy();
      virtual ~FEROLproxy() {};
      
      virtual void dataReadyCallback(toolbox::mem::Reference*, pt::utcp::frl::MemoryCache*);
      virtual bool getNextAvailableSuperFragment(FragmentChainPtr&);
      virtual bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&);
      virtual void configure(const Configuration&);
      virtual void startProcessing(const uint32_t runNumber);
      virtual void clear();
      uint32_t extractTriggerInformation(const unsigned char* payload) const;
      
    private:

      void addFragment(toolbox::mem::Reference*);
      toolbox::mem::Reference* copyDataIntoDataBlock(FragmentChainPtr);
      void fillBlockInfo(toolbox::mem::Reference*, const EvBid&, const uint32_t nbBlocks) const;
      
      bool dropInputData_;
      uint16_t triggerFedId_;
      
      FragmentChain::ResourceList fedList_;
      typedef std::map<EvBid,FragmentChainPtr> SuperFragmentMap;
      SuperFragmentMap superFragmentMap_;
      boost::mutex superFragmentMapMutex_;
      
      typedef std::map<uint16_t,EvBidFactory> EvBidFactories;
      EvBidFactories evbIdFactories_;

      uint32_t nbSuperFragmentsReady_;
    };
    
    
    class DummyInputData : public InputHandler
    {
    public:
      
      DummyInputData();
      virtual ~DummyInputData() {};

      virtual void dataReadyCallback(toolbox::mem::Reference*, pt::utcp::frl::MemoryCache*);
      virtual bool getNextAvailableSuperFragment(FragmentChainPtr&);
      virtual bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&);
      virtual void configure(const Configuration&);
      virtual void startProcessing(const uint32_t runNumber);
      
    private:

      FragmentChain::ResourceList fedList_;
      EvBidFactory evbIdFactory_;
      typedef std::map<uint16_t,FragmentTracker> FragmentTrackers;
      FragmentTrackers fragmentTrackers_;
      toolbox::mem::Pool* fragmentPool_;
      uint32_t eventNumber_;
      
    };
    
  } } //namespace evb::ru


#endif // _evb_ru_InputHandler_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
