#ifndef _evb_bu_RUproxy_h_
#define _evb_bu_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/EvBid.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/bu/Configuration.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb {

  class BU;
  
  namespace bu { // namespace evb::bu
    
    class ResourceManager;
    class StateMachine;

    typedef FragmentChain<msg::I2O_DATA_BLOCK_MESSAGE_FRAME> FragmentChain;
    typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;
    
    /**
     * \ingroup xdaqApps
     * \brief Proxy for EVM-BU communication
     */
    
    class RUproxy : public toolbox::lang::Class
    {
      
    public:
      
      RUproxy
      (
        BU*,
        boost::shared_ptr<ResourceManager>,
        toolbox::mem::Pool*
      );
      
      virtual ~RUproxy() {};
      
      /**
       * Callback for I2O message containing a super fragment
       */
      void superFragmentCallback(toolbox::mem::Reference*);
      
      /**
       * Fill the next available data fragment
       * into the passed buffer reference.
       * Return false if no data is available
       */
      bool getData(toolbox::mem::Reference*&);
      
      /**
       * Send request for N trigger data fragments to the RUs
       */
      void requestTriggerData(const uint32_t buResourceId, const uint32_t count);
      
      /**
       * Send request for event fragments for the given
       * event-builder ids to the RUS
       */
      void requestFragments(const uint32_t buResourceId, const EvBids&);
      
      /**
       * Append the info space items to be published in the
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);
      
      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();
      
      /**
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();
      
      /**
       * Configure
       */
      void configure();
      
      /**
       * Find the application descriptors of the participating EVM and RUs.
       */
      void getApplicationDescriptors();
      
      /**
       * Remove all data
       */
      void clear();
      
      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }
      
      /**
       * Start processing messages
       */
      void startProcessing();

      /**
       * Stop processing messages
       */
      void stopProcessing();
      
      /**
       * Return the tids of RUs participating in the event building
       */
      std::vector<I2O_TID>& getRuTids()
      { return ruTids_; }
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      /**
       * Print the content of the fragment FIFO as HTML snipped
       */
      inline void printFragmentFIFO(xgi::Output* out)
      { fragmentFIFO_.printVerticalHtml(out); }
      
      
    private:
      
      void startProcessingWorkLoops();
      bool requestTriggers(toolbox::task::WorkLoop*);
      bool requestFragments(toolbox::task::WorkLoop*);
      void sendToAllRUs(toolbox::mem::Reference*, const size_t bufSize) const;
      void getApplicationDescriptorForEVM();
      void getApplicationDescriptorsForRUs();
      
      BU* bu_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;

      toolbox::mem::Pool* fastCtrlMsgPool_;
      const ConfigurationPtr configuration_;      
      
      bool doProcessing_;
      bool requestTriggersActive_;
      bool requestFragmentsActive_;
      
      toolbox::task::WorkLoop* requestTriggersWL_;
      toolbox::task::ActionSignature* requestTriggersAction_;
      toolbox::task::WorkLoop* requestFragmentsWL_;
      toolbox::task::ActionSignature* requestFragmentsAction_;
      
      I2O_TID tid_;
      ApplicationDescriptorAndTid evm_;
      ApplicationDescriptorsAndTids participatingRUs_;
      std::vector<I2O_TID> ruTids_;
      
      typedef OneToOneQueue<toolbox::mem::Reference*> FragmentFIFO;
      FragmentFIFO fragmentFIFO_;
      
      // Lookup table of data blocks, indexed by RU tid and BU resource id
      struct Index
      {
        uint32_t ruTid;
        uint32_t buResourceId;
        
        inline bool operator< (const Index& other) const
        { return ruTid == other.ruTid ? buResourceId < other.buResourceId : ruTid < other.ruTid; }
      };
      typedef std::map<Index,FragmentChainPtr> DataBlockMap;
      DataBlockMap dataBlockMap_;
      
      typedef std::map<uint32_t,uint64_t> CountsPerRU;
      struct FragmentMonitoring
      {
        uint32_t lastEventNumberFromEVM;
        uint32_t lastEventNumberFromRUs;
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        CountsPerRU logicalCountPerRU;
        CountsPerRU payloadPerRU;
      } fragmentMonitoring_;
      boost::mutex fragmentMonitoringMutex_;
      
      struct TriggerRequestMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } triggerRequestMonitoring_;
      boost::mutex triggerRequestMonitoringMutex_;
      
      struct FragmentRequestMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } fragmentRequestMonitoring_;
      boost::mutex fragmentRequestMonitoringMutex_;
      
      xdata::UnsignedInteger64 i2oBUCacheCount_;
      xdata::UnsignedInteger64 i2oRUSendCount_;
    };
    
    
  } //namespace evb::bu

  template <>
  inline void OneToOneQueue<toolbox::mem::Reference*>::formatter(toolbox::mem::Reference* bufRef, std::ostringstream* out)
  {
    const msg::I2O_DATA_BLOCK_MESSAGE_FRAME* msg = (msg::I2O_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
    if ( msg )
      *out << *msg;
    else
      *out << "n/a";
  }


} //namespace evb

#endif // _evb_bu_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
