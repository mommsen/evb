#ifndef _evb_ru_InputHandler_h_
#define _evb_ru_InputHandler_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>
#include <vector>

#include "i2o/shared/i2omsg.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/OneToOneQueue.h"
#include "evb/SuperFragmentGenerator.h"
#include "evb/ru/SuperFragmentTable.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {
  
  class RU;
  
  namespace ru {
    
    class InputHandler
    {
    public:
      
      InputHandler(boost::shared_ptr<RU> ru, SuperFragmentTablePtr sft) :
      ru_(ru), superFragmentTable_(sft) {};
      
      virtual ~InputHandler() {};
      
      /**
       * Callback for I2O message received from frontend
       */
      virtual void I2Ocallback(toolbox::mem::Reference*) = 0;

      /**
       * Configure
       */
      struct Configuration
      {
        uint32_t dummyFedPayloadSize;
        uint32_t dummyFedPayloadStdDev;
        xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;
        bool usePlayback;
        std::string playbackDataFile;
      };
      virtual void configure(const Configuration&) {};
      
      /**
       * Remove all data
       */
      virtual void clear() {};
      
      /**
       * Print monitoring information as HTML snipped
       */
      virtual void printHtml(xgi::Output*) = 0;
      
      /**
       * Return the last event number seen
       */
      inline uint32_t lastEventNumber() const
      { return inputMonitoring_.lastEventNumber; }
      
      /**
       * Return the number of received event fragments
       * since the last call to resetMonitoringCounters
       */
      inline uint64_t fragmentsCount() const
      { return inputMonitoring_.logicalCount; }
      
      /**
       * Reset the monitoring counters
       */
      inline void resetMonitoringCounters()
      {
        boost::mutex::scoped_lock sl(inputMonitoringMutex_);
        
        inputMonitoring_.payload = 0;
        inputMonitoring_.logicalCount = 0;
        inputMonitoring_.i2oCount = 0;
        inputMonitoring_.lastEventNumber = 0;
      }
      
    protected:
      
      boost::shared_ptr<RU> ru_;
      SuperFragmentTablePtr superFragmentTable_;
      
      struct InputMonitoring
      {
        uint64_t logicalCount;
        uint64_t payload;
        uint64_t i2oCount;
        uint32_t lastEventNumber;
      } inputMonitoring_;
      boost::mutex inputMonitoringMutex_;
    };
    
    
    class FEROLproxy : public InputHandler
    {
    public:
      
      FEROLproxy(boost::shared_ptr<RU>, SuperFragmentTablePtr);
      virtual ~FEROLproxy() {};
      
      virtual void I2Ocallback(toolbox::mem::Reference*);
      virtual void configure(const Configuration&);
      virtual void clear();
      virtual void printHtml(xgi::Output*);
      
    private:
      
      void updateInputCounters(toolbox::mem::Reference*);
      
    };
    
    
    class DummyInputData : public InputHandler
    {
    public:
      
      DummyInputData(boost::shared_ptr<RU>, SuperFragmentTablePtr);
      virtual ~DummyInputData() {};

      virtual void I2Ocallback(toolbox::mem::Reference*);
      virtual void configure(const Configuration&);
      virtual void printHtml(xgi::Output*);
      
    private:
      
      toolbox::mem::Reference* generateDummySuperFrag(const uint32_t eventNumber);
      void setNbBlocksInSuperFragment
      (
      toolbox::mem::Reference*,
      const uint32_t nbBlocks
      );
      
      evb::SuperFragmentGenerator superFragmentGenerator_;
    };
    
  } } //namespace evb::ru


#endif // _evb_ru_InputHandler_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
