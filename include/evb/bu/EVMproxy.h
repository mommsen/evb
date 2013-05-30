#ifndef _evb_bu_EVMproxy_h_
#define _evb_bu_EVMproxy_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/TimerManager.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Integer32.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb { namespace bu { // namespace evb::bu

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-BU communication
   */
  
  class EVMproxy
  {
    
  public:

    EVMproxy
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    virtual ~EVMproxy() {};
    
    /**
     * Callback for I2O message received from EVM
     */
    void I2Ocallback(toolbox::mem::Reference*);
    
    /**
     * Fill the next available trigger block
     * into the passed buffer reference.
     * Return false if no trigger block is available
     */
    bool getTriggerBlock(toolbox::mem::Reference*&);
    
    /**
     * Send the request for and/or release of event ids
     * to the EVM
     */
    void sendEvtIdRqstAndOrRelease(const msg::EvtIdRqstAndOrRelease&);

    /**
     * Send any old, but incomplete request message to the EVM.
     * Returns true if a message was sent.
    */
    bool sendOldRequests();
    
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
    void configure(const int msgAgeLimitDtMSec);
    
    /**
     * Find the application descriptors of the participating EVM.
     *
     * By default, the EVM found in the default zone will be used.
     * This can be overwritten by setting the EVM instance
     * in 'evmInstance'.
     */
    void getApplicationDescriptors();
    
    /**
     * Return the EVM instance used for communicating
     */
    int32_t getEvmInstance()
    { return evm_.descriptor ? evm_.descriptor->getInstance() : -1; }

    /**
     * Remove all data
     */
    void clear();
  
    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the trigger FIFO as HTML snipped
     */
    inline void printTriggerFIFO(xgi::Output* out)
    { triggerFIFO_.printVerticalHtml(out); }


  private:
    
    void updateTriggerCounters(toolbox::mem::Reference*);
    void dumpTriggersToLogger(toolbox::mem::Reference*);
    void createEvtIdRqstsAndOrReleaseMsg();
    uint32_t packEvtIdRqstsAndOrReleasesMsg(const msg::EvtIdRqstAndOrRelease&);
    void sendEvtIdRqstAndOrReleaseToEVM();
    void updateRqstAndOrReleaseCounters();
    
    xdaq::Application* app_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    I2O_TID tid_;
    uint32_t index_;
    ApplicationDescriptorAndTid evm_;

    typedef OneToOneQueue<toolbox::mem::Reference*> TriggerFIFO;
    TriggerFIFO triggerFIFO_;
    
    toolbox::mem::Reference* evtIdRqstsAndOrReleasesBufRef_;
    boost::mutex evtIdRqstsAndOrReleasesMutex_;
    size_t evtIdRqstsAndOrReleasesBufSize_;
    
    TimerManager timerManager_;
    const uint8_t timerId_;
    
    struct TriggerMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumberFromEVM;
    } triggerMonitoring_;
    boost::mutex triggerMonitoringMutex_;
    
    struct RqstAndOrReleaseMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
    } rqstAndOrReleaseMonitoring_;
    boost::mutex rqstAndOrReleaseMonitoringMutex_;
    
    InfoSpaceItems evmParams_;
    xdata::UnsignedInteger32 nbEvtIdsInBuilder_;
    xdata::UnsignedInteger32 I2O_EVM_ALLOCATE_CLEAR_Packing_;
    xdata::Integer32 evmInstance_;
    xdata::Boolean dumpTriggersToLogger_;
    
    xdata::UnsignedInteger32 lastEventNumberFromEVM_;
    xdata::UnsignedInteger64 i2oBUConfirmCount_;
    xdata::UnsignedInteger64 i2oEVMAllocCount_;
  };
  
  
} } //namespace evb::bu

#endif // _evb_bu_EVMproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
