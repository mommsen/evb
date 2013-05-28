#ifndef _evb_ru_BUproxy_h_
#define _evb_ru_BUproxy_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>
#include <set>
#include <vector>

#include "evb/EvBid.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/ru/Input.h"
#include "i2o/i2oDdmLib.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {
  
  class RU;
  
  namespace ru {

    class StateMachine;
    
    
    /**
     * \ingroup xdaqApps
     * \brief Proxy for RU-BU communication
     */
    
    class BUproxy :  public toolbox::lang::Class
    {
      
    public:
      
      BUproxy
      (
        RU*,
        boost::shared_ptr<ru::Input>
      );
      
      virtual ~BUproxy() {};
      
      /**
       * Callback for fragment request I2O message from BU
       */
      void rqstForFragmentsMsgCallback(toolbox::mem::Reference*);
      
      /**
       * Configure the BU proxy
       */
      void configure();
      
      /**
       * Append the info space parameters used for the
       * configuration to the InfoSpaceItems
       */
      void appendConfigurationItems(InfoSpaceItems&);
      
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
       * Remove all data
       */
      void clear();
      
      /**
       * Start processing messages
       */
      void startProcessing();
      
      /**
       * Stop processing messages
       */
      void stopProcessing();
      
      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }
      
      /**
       * Return the logical number of I2O_BU_CACHE messages
       * received since the last call to resetMonitoringCounters
       */
      uint64_t i2oBUCacheCount() const
      { return dataMonitoring_.logicalCount; }
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      struct Request
      {
        I2O_TID  buTid;
        uint32_t buResourceId;
        uint32_t nbRequests;
        std::vector<EvBid> evbIds;
      };
      
      
    private:
      
      typedef std::vector<FragmentChainPtr> SuperFragments;
      
      void startProcessingWorkLoop();
      void updateRequestCounters(const msg::RqstForFragmentsMsg*);
      bool processSuperFragments(toolbox::task::WorkLoop*);
      bool processTriggers(toolbox::task::WorkLoop*);
      void sendData(const Request&, const SuperFragments&);
      toolbox::mem::Reference* getNextBlock(const uint32_t blockNb);
      void fillSuperFragmentHeader
      (
        unsigned char*& payload,
        size_t& remainingPayloadSize,
        const uint32_t nbSuperFragments,
        const uint32_t superFragmentNb,
        const FragmentChainPtr
      ) const;
      
      RU* ru_;
      boost::shared_ptr<ru::Input> input_;
      boost::shared_ptr<StateMachine> stateMachine_;
      uint32_t tid_;
      toolbox::mem::Pool* superFragmentPool_;
      
      toolbox::task::WorkLoop* processingWL_;
      toolbox::task::ActionSignature* fragmentAction_;
      toolbox::task::ActionSignature* triggerAction_;
      bool doProcessing_;
      bool processActive_;

      typedef OneToOneQueue<Request> RequestFIFO;
      RequestFIFO requestFIFO_;
      
      InfoSpaceItems buParams_;
      typedef std::map<uint32_t,uint64_t> CountsPerBU;
      struct RequestMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        CountsPerBU logicalCountPerBU;
      } requestMonitoring_;
      boost::mutex requestMonitoringMutex_;
      
      struct DataMonitoring
      {
        uint32_t lastEventNumberToBUs;
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        CountsPerBU payloadPerBU;
      } dataMonitoring_;
      boost::mutex dataMonitoringMutex_;
      
      xdata::UnsignedInteger32 lastEventNumberToBUs_;
      xdata::UnsignedInteger32 nbSuperFragmentsReady_;
      xdata::UnsignedInteger64 i2oBUCacheCount_;
      xdata::Vector<xdata::UnsignedInteger64> i2oRUSendCountBU_;
      xdata::Vector<xdata::UnsignedInteger64> i2oBUCachePayloadBU_;

      xdata::UnsignedInteger32 maxTriggerAgeMSec_;
      xdata::UnsignedInteger32 blockSize_;
      xdata::UnsignedInteger32 requestFIFOCapacity_;
      xdata::Boolean hasTriggerFED_;

    };
    
  } } //namespace evb::ru


std::ostream& operator<<(std::ostream&, const evb::ru::BUproxy::Request&);

#endif // _evb_ru_BUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
