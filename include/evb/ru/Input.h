#ifndef _evb_ru_input_h_
#define _evb_ru_input_h_

#include <boost/scoped_ptr.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "evb/EvBid.h"
#include "evb/InfoSpaceItems.h"
#include "evb/ru/InputHandler.h"
#include "evb/ru/SuperFragment.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {

  class RU;
  
  namespace ru {
    
    /**
     * \ingroup xdaqApps
     * \brief Event fragment input handler of RU
     */
    
    class Input
    {
      
    public:
      
      Input(RU*);
      
      virtual ~Input() {};
      
      /**
       * Notify the proxy of an input source change.
       * The input source is taken from the info space
       * parameter 'inputSource'.
       *
       * Valid input strings are:
       * FEROL: FED fragments are sent from the FEROL
       *        in form of a I2O_DATA_READY_MESSAGE_FRAME message
       * Local: dummy data is generated locally 
       */
      void inputSourceChanged();
      
      /**
       * Enable or disable the handling of incoming I2O messages
       */
      void acceptI2Omessages(const bool accept)
      { acceptI2Omessages_ = accept; }
      
      /**
       * Callback for I2O_DATA_READY message received from frontend
       */
      void dataReadyCallback(toolbox::mem::Reference*);
      
      /**
       * Get the next complete super fragment.
       * If none is available, the method returns false.
       * Otherwise, the SuperFragmentPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getNextAvailableSuperFragment(SuperFragmentPtr);
      
      /**
       * Get the complete super fragment with EvBid.
       * If it is not available or complete, the method returns false.
       * Otherwise, the SuperFragmentPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getSuperFragmentWithEvBid(const EvBid&, SuperFragmentPtr);
            
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
       * Remove all data
       */
      void clear();
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      
    private:
      
      void dumpFragmentToLogger(toolbox::mem::Reference*) const;
      void updateInputCounters(toolbox::mem::Reference*);
      
      RU* ru_;
      boost::scoped_ptr<InputHandler> handler_;
      bool acceptI2Omessages_;
      
      struct InputMonitoring
      {
        uint32_t lastEventNumber;
        PerformanceMonitor perf;
        double rate;
        double bandwidth;
      };
      typedef std::map<uint16_t,InputMonitoring> InputMonitors;
      InputMonitors inputMonitors_;
      boost::mutex inputMonitorsMutex_;
      
      InfoSpaceItems inputParams_;
      xdata::String inputSource_;
      xdata::Boolean dumpFragmentsToLogger_;
      xdata::Boolean dropInputData_;
      xdata::Boolean usePlayback_;
      xdata::String playbackDataFile_;
      xdata::UnsignedInteger32 dummyFedSize_;
      xdata::UnsignedInteger32 dummyFedSizeStdDev_;
      xdata::UnsignedInteger32 fragmentPoolSize_;
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds_;
      
      xdata::UnsignedInteger32 lastEventNumberFromRUI_;
      xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;
    };
    
    
  } } //namespace evb::ru


#endif // _evb_ru_input_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
