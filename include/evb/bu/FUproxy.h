#ifndef _evb_bu_FUproxy_h_
#define _evb_bu_FUproxy_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>
#include <set>

#include "i2o/i2oDdmLib.h"
#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/FuRqstForResource.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb {
  namespace bu {
    
    /**
     * \ingroup xdaqApps
     * \brief Proxy for EVM-BU communication
     */
    
    class FUproxy
    {
      
    public:
      
      FUproxy
      (
        xdaq::Application*,
        toolbox::mem::Pool*
      );
      
      virtual ~FUproxy() {};
      
      /**
       * Callback for allocate I2O message received from FU
       */
      void allocateI2Ocallback(toolbox::mem::Reference*);
      
      /**
       * Callback for collect I2O message received from FU.
       * This I2O message is not supported in this version.
       */
      void collectI2Ocallback(toolbox::mem::Reference*);
      
      /**
       * Callback for discard I2O message received from FU
       */
      void discardI2Ocallback(toolbox::mem::Reference*);
      
      /**
       * Handle the end-of-lumisection I2O message received from EVM
       */
      void handleEoLSmsg(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*);
      
      /**
       * Fill the next available request for an event
       * into the passed request struct.
       * Return false if no request is available
       */
      bool getNextRequest(FuRqstForResource&);
      
      /**
       * Send the passed super fragment to the FU 
       * given in the request
       */
      void sendSuperFragment
      (
        const FuRqstForResource&,
        const uint32_t superFragmentNb,
        const uint32_t nbSuperFragmentsInEvent,
        toolbox::mem::Reference*
      );
      
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
       * Configure
       */
      void configure();
      
      /**
       * Find the application descriptors of the participating EVM.
       *
       * By default, the EVM found in the default zone will be used.
       * This can be overwritten by setting the EVM instance
       * in 'evmInstance'.
       */
      void getApplicationDescriptors();
      
      /**
       * Register the event table
       */
      void registerEventTable(EventTablePtr eventTable)
      { eventTable_ = eventTable; }
      
      /**
       * Remove all data
       */
      void clear();
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      /**
       * Print the content of the request FIFO as HTML snipped
       */
      inline void printRequestFIFO(xgi::Output* out)
      { requestFIFO_.printVerticalHtml(out); }
      
      
    private:
      
      void updateAllocateCounters(const uint32_t nbElements);
      void updateParticipatingFUs(const I2O_TID&);
      void pushRequestOntoFIFO
      (
        const I2O_BU_ALLOCATE_MESSAGE_FRAME*,
        const I2O_TID& fuTid
      );
      void updateDiscardCounters(const uint32_t nbElements);
      void broadcastEoLSmsg(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*);
      
      xdaq::Application* app_;
      toolbox::mem::Pool* fastCtrlMsgPool_;
      log4cplus::Logger& logger_;
      uint32_t tid_;
      
      EventTablePtr eventTable_;
      
      typedef OneToOneQueue<FuRqstForResource> RequestFIFO;
      RequestFIFO requestFIFO_;
      
      typedef std::set<I2O_TID> ParticipatingFUs;
      ParticipatingFUs participatingFUs_;
      boost::mutex participatingFUsMutex_;
      
      struct AllocateMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } allocateMonitoring_;
      boost::mutex allocateMonitoringMutex_;
      
      struct DiscardMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
      } discardMonitoring_;
      boost::mutex discardMonitoringMutex_;
      
      struct DataMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        uint32_t lastEventNumberToFUs;
      } dataMonitoring_;
      boost::mutex dataMonitoringMutex_;
    
      struct EoLSMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        uint32_t lastLumiSection;
      } eolsMonitoring_;
      boost::mutex eolsMonitoringMutex_;
      
      InfoSpaceItems fuParams_;
      xdata::UnsignedInteger32 requestFIFOCapacity_;
      
      xdata::UnsignedInteger64 i2oBUAllocCount_;
      xdata::UnsignedInteger64 i2oBUDiscardCount_;
      xdata::UnsignedInteger64 i2oFUTakeCount_;
      xdata::UnsignedInteger32 lastEventNumberToFUs_;
    };
    
  } } //namespace evb::bu

#endif // _evb_bu_FUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
