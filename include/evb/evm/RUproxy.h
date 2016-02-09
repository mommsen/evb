#ifndef _evb_evm_RUproxy_h_
#define _evb_evm_RUproxy_h_

#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "cgicc/HTMLClasses.h"
#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/readoutunit/FragmentRequest.h"
#include "evb/readoutunit/StateMachine.h"
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

  class EVM;

  namespace evm { // namespace evb::evm

    /**
     * \ingroup xdaqApps
     * \brief Proxy for EVM-RU communication
     */

    class RUproxy : public toolbox::lang::Class
    {

    public:

      RUproxy
      (
        EVM*,
        boost::shared_ptr< readoutunit::StateMachine<EVM> >,
        toolbox::mem::Pool*
      );

      ~RUproxy();

      /**
       * Send the request for event fragments to all RUs
       */
      void sendRequest(const readoutunit::FragmentRequestPtr&);

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
       * Start processing events
       */
      void startProcessing();

      /**
       * Drain events
       */
      void drain() const;

      /**
       * Stop processing events
       */
      void stopProcessing();

      /**
       * Return true if no work is being done
       */
      bool isEmpty() const;

      /**
       * Return the tids of RUs participating in the event building
       */
      std::vector<I2O_TID>& getRUtids()
      { return ruTids_; }

      /**
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;


    private:

      void resetMonitoringCounters();
      void startRequestWorkLoop();
      toolbox::mem::Reference* getRequestMsgBuffer(const uint32_t bufSize);
      void sendMsgToRUs(toolbox::mem::Reference*&, const uint32_t msgSize, uint32_t& requestCount);
      bool processRequests(toolbox::task::WorkLoop*);
      void getApplicationDescriptors();
      void fillRUInstance(xdata::UnsignedInteger32 instance);

      EVM* evm_;
      boost::shared_ptr< readoutunit::StateMachine<EVM> > stateMachine_;

      toolbox::mem::Pool* fastCtrlMsgPool_;

      typedef OneToOneQueue<readoutunit::FragmentRequestPtr> ReadoutMsgFIFO;
      ReadoutMsgFIFO readoutMsgFIFO_;

      toolbox::task::WorkLoop* processRequestsWL_;
      toolbox::task::ActionSignature* processRequestsAction_;
      volatile bool doProcessing_;
      volatile bool processingActive_;

      I2O_TID tid_;
      uint32_t ruCount_;

      typedef std::vector<ApplicationDescriptorAndTid> ApplicationDescriptorsAndTids;
      ApplicationDescriptorsAndTids participatingRUs_;
      std::vector<I2O_TID> ruTids_;

      struct AllocateMonitoring
      {
        uint32_t lastEventNumberToRUs;
        uint64_t bandwidth;
        uint32_t assignmentRate;
        uint32_t i2oRate;
        double packingFactor;
        PerformanceMonitor perf;
     } allocateMonitoring_;
      mutable boost::mutex allocateMonitoringMutex_;

    };


  } //namespace evb::evm


} //namespace evb

#endif // _evb_evm_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
