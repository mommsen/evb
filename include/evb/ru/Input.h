#ifndef _evb_ru_input_h_
#define _evb_ru_input_h_

#include <boost/scoped_ptr.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "evb/EvBid.h"
#include "evb/InfoSpaceItems.h"
#include "evb/ru/InputHandler.h"
#include "evb/ru/SuperFragmentTable.h"
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
      
      Input(boost::shared_ptr<RU>, SuperFragmentTablePtr);
      
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
       * Callback for I2O message received from frontend
       */
      void I2Ocallback(toolbox::mem::Reference*);
      
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
       * Return the logical number of I2O_EVMRU_DATA_READY messages
       * received since the last call to resetMonitoringCounters
       */
      uint64_t fragmentsCount() const;
      
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
      
      boost::shared_ptr<RU> ru_;
      SuperFragmentTablePtr superFragmentTable_;
      boost::scoped_ptr<InputHandler> handler_;
      
      InfoSpaceItems inputParams_;
      xdata::String inputSource_;
      xdata::Boolean dumpFragmentsToLogger_;
      xdata::Boolean usePlayback_;
      xdata::String playbackDataFile_;
      xdata::UnsignedInteger32 dummyFedPayloadSize_;
      xdata::UnsignedInteger32 dummyFedPayloadStdDev_;
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
