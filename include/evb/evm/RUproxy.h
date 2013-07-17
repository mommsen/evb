#ifndef _evb_evm_RUproxy_h_
#define _evb_evm_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/EvBid.h"
#include "evb/FragmentChain.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
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
        toolbox::mem::Pool*
      );

      virtual ~RUproxy() {};

      /**
       * Send the request for event fragments to all RUs
       */
      void sendRequest(const readoutunit::FragmentRequestPtr);

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
       * Remove all data
       */
      void clear();

      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr< readoutunit::StateMachine<EVM> > stateMachine)
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
      std::vector<I2O_TID>& getRUtids()
      { return ruTids_; }

      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);

      /**
       * Print the content of the allocation FIFO as HTML snipped
       */
      inline void printAllocateFIFO(xgi::Output* out)
      { allocateFIFO_.printVerticalHtml(out); }


    private:

      void startProcessingWorkLoop();
      bool assignEvents(toolbox::task::WorkLoop*);
      void sendToAllRUs(toolbox::mem::Reference*, const size_t bufSize) const;
      void getApplicationDescriptorsForRUs();

      EVM* evm_;
      boost::shared_ptr< readoutunit::StateMachine<EVM> > stateMachine_;

      toolbox::mem::Pool* fastCtrlMsgPool_;

      typedef OneToOneQueue<readoutunit::FragmentRequestPtr> AllocateFIFO;
      AllocateFIFO allocateFIFO_;

      bool doProcessing_;
      bool assignEventsActive_;

      toolbox::task::WorkLoop* assignEventsWL_;
      toolbox::task::ActionSignature* assignEventsAction_;

      I2O_TID tid_;
      ApplicationDescriptorsAndTids participatingRUs_;
      std::vector<I2O_TID> ruTids_;

      struct AllocateMonitoring
      {
        uint32_t lastEventNumberToRUs;
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } allocateMonitoring_;
      boost::mutex allocateMonitoringMutex_;

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
