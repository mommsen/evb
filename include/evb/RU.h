#ifndef _evb_ru_h_
#define _evb_ru_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

#include "evb/EvBApplication.h"
#include "evb/PerformanceMonitor.h"
#include "evb/ru/StateMachine.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoop.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/Double.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb {
  
  class InfoSpaceItems;
  class TimerManager;
  
  namespace ru {
    class BUproxy;
    class Input;
    class SuperFragmentTable;
  }

  /**
   * \ingroup xdaqApps
   * \brief Readout unit (RU)
   */
  class RU : public EvBApplication<ru::StateMachine>, public boost::enable_shared_from_this<RU>
  {
  public:
    
    RU(xdaq::ApplicationStub*);
    
    virtual ~RU() {};  
    
    XDAQ_INSTANTIATOR();
    
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
     * Start (local) triggers and process messages
     */
    void startProcessing();
    
    /**
     * Stop processing messages
     */
    void stopProcessing();
    
    
  private:
    
    virtual void bindI2oCallbacks();
    inline void I2O_EVMRU_DATA_READY_Callback(toolbox::mem::Reference*);
    inline void I2O_RU_SEND_Callback(toolbox::mem::Reference*);
    
    virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
    virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
    virtual void do_updateMonitoringInfo();
    
    virtual void do_handleItemChangedEvent(const std::string& item);
    virtual void do_handleItemRetrieveEvent(const std::string& item);
    
    virtual void bindNonDefaultXgiCallbacks();
    virtual void do_defaultWebPage(xgi::Output*);
    void printHtml(xgi::Output*);
    
    void startProcessingWorkLoop();
    bool process(toolbox::task::WorkLoop*);
    void updateSuperFragmentCounters(toolbox::mem::Reference*);
    void getPerformance(PerformanceMonitor&);
    
    boost::shared_ptr<ru::BUproxy> buProxy_;
    boost::shared_ptr<ru::Input> ruInput_;
    boost::shared_ptr<ru::SuperFragmentTable> superFragmentTable_;
    
    volatile bool doProcessing_;
    volatile bool processActive_;
    
    toolbox::task::WorkLoop* processingWL_;
    toolbox::task::ActionSignature* processingAction_;
    
    TimerManager timerManager_;
    const uint8_t timerId_;
    
    struct SuperFragmentMonitoring
    {
      uint64_t count;
      uint64_t payload;
      uint64_t payloadSquared;
    } superFragmentMonitoring_;
    boost::mutex superFragmentMonitoringMutex_;
    
    PerformanceMonitor intervalStart_;
    PerformanceMonitor delta_;
    boost::mutex performanceMonitorMutex_;
    
    xdata::Double deltaT_;
    xdata::UnsignedInteger32 deltaN_;
    xdata::UnsignedInteger64 deltaSumOfSquares_;
    xdata::UnsignedInteger32 deltaSumOfSizes_;
    
    xdata::UnsignedInteger32 runNumber_;
    xdata::UnsignedInteger32 maxPairAgeMSec_;
    
    xdata::UnsignedInteger32 monitoringRunNumber_;
    xdata::UnsignedInteger32 nbSuperFragmentsInRU_;
    
    xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;
    xdata::UnsignedInteger64 i2oBUCacheCount_;
    xdata::UnsignedInteger32 nbSuperFragmentsReady_;
    
  }; // class RU
  
} // namespace evb

#endif // _evb_ru_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
