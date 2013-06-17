#ifndef _evb_ru_h_
#define _evb_ru_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

#include "evb/EvBApplication.h"
#include "evb/PerformanceMonitor.h"
#include "evb/ru/StateMachine.h"
#include "pt/utcp/frl/MemoryCache.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoop.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
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
  class RU : public EvBApplication<ru::StateMachine>
  {
  public:
    
    RU(xdaq::ApplicationStub*);
    
    virtual ~RU() {};  
    
    XDAQ_INSTANTIATOR();
    
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
    inline void I2O_DATA_READY_Callback(toolbox::mem::Reference*);
    inline void I2O_RU_SEND_Callback(toolbox::mem::Reference*);
    inline void rawDataAvailable(toolbox::mem::Reference*, int originator, pt::utcp::frl::MemoryCache*);

    virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
    virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
    virtual void do_updateMonitoringInfo();
    
    virtual void do_handleItemChangedEvent(const std::string& item);
    virtual void do_handleItemRetrieveEvent(const std::string& item);
    
    virtual void bindNonDefaultXgiCallbacks();
    virtual void do_defaultWebPage(xgi::Output*);
    
    void requestFIFOWebPage(xgi::Input*, xgi::Output*);
    
    boost::shared_ptr<ru::BUproxy> buProxy_;
    boost::shared_ptr<ru::Input> ruInput_;
    boost::shared_ptr<ru::SuperFragmentTable> superFragmentTable_;
    
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
