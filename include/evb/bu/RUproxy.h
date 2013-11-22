#ifndef _evb_bu_RUproxy_h_
#define _evb_bu_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <curl/curl.h>
#include <map>
#include <stdint.h>

#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/EvBid.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/bu/Configuration.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {

  class BU;

  namespace bu { // namespace evb::bu

    class EventBuilder;
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
        boost::shared_ptr<EventBuilder>,
        boost::shared_ptr<ResourceManager>,
        toolbox::mem::Pool*
      );

      ~RUproxy();

      /**
       * Callback for I2O message containing a super fragment
       */
      void superFragmentCallback(toolbox::mem::Reference*);

      /**
       * Send request for N trigger data fragments to the RUs
       */
      void requestFragments(const uint32_t buResourceId, const uint32_t count);

      /**
       * Get the total number of events in the given lumi section from the EVM
       */
      uint32_t getTotalEventsInLumiSection(const uint32_t lumiSection);

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
       * Configure
       */
      void configure();

      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }

      /**
       * Start processing events
       */
      void startProcessing();

      /**
       * Drain events
       */
      void drain();

      /**
       * Stop processing events
       */
      void stopProcessing();

      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*) const;


    private:

      void resetMonitoringCounters();
      void getApplicationDescriptors();
      void startProcessingWorkLoop();
      bool requestFragments(toolbox::task::WorkLoop*);
      void getApplicationDescriptorForEVM();
      static int curlWriter(char*, size_t, size_t, std::string*);

      BU* bu_;
      boost::shared_ptr<EventBuilder> eventBuilder_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;

      toolbox::mem::Pool* fastCtrlMsgPool_;
      const ConfigurationPtr configuration_;

      bool doProcessing_;
      bool requestFragmentsActive_;

      toolbox::task::WorkLoop* requestFragmentsWL_;
      toolbox::task::ActionSignature* requestFragmentsAction_;

      I2O_TID tid_;
      ApplicationDescriptorAndTid evm_;
      std::string evmURL_;
      CURL* curl_;
      std::string curlBuffer_;

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
      mutable boost::mutex fragmentMonitoringMutex_;

      struct RequestMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } requestMonitoring_;
      mutable boost::mutex requestMonitoringMutex_;

      xdata::UnsignedInteger64 requestCount_;
      xdata::UnsignedInteger64 fragmentCount_;
      xdata::Vector<xdata::UnsignedInteger64> fragmentCountPerRU_;
      xdata::Vector<xdata::UnsignedInteger64> payloadPerRU_;
    };


  } //namespace evb::bu


} //namespace evb

#endif // _evb_bu_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
