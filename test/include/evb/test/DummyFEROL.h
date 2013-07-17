#ifndef _evb_test_DummyFEROL_h_
#define _evb_test_DummyFEROL_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "evb/EvBApplication.h"
#include "evb/FragmentGenerator.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/test/dummyFEROL/StateMachine.h"
#include "i2o/i2oDdmLib.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {
  namespace test {

    /**
     * \ingroup xdaqApps
     * \brief The dummy FEROL
     */

    class DummyFEROL : public EvBApplication<dummyFEROL::StateMachine>
    {

    public:

      DummyFEROL(xdaq::ApplicationStub*);

      virtual ~DummyFEROL() {};

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
       * Start triggers and send messages
       */
      void startProcessing();

      /**
       * Stop triggers and sending messages
       */
      void stopProcessing();


    private:

      virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
      virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
      virtual void do_updateMonitoringInfo();

      virtual void bindNonDefaultXgiCallbacks();
      virtual void do_defaultWebPage(xgi::Output*);
      void fragmentFIFOWebPage(xgi::Input*, xgi::Output*);

      void getApplicationDescriptors();
      void startWorkLoops();
      bool generating(toolbox::task::WorkLoop*);
      bool sending(toolbox::task::WorkLoop*);
      void updateCounters(toolbox::mem::Reference*);
      void sendData(toolbox::mem::Reference*);
      void getPerformance(PerformanceMonitor&);

      xdaq::ApplicationDescriptor* ruDescriptor_;

      volatile bool doProcessing_;
      volatile bool generatingActive_;
      volatile bool sendingActive_;

      toolbox::task::WorkLoop* generatingWL_;
      toolbox::task::WorkLoop* sendingWL_;
      toolbox::task::ActionSignature* generatingAction_;
      toolbox::task::ActionSignature* sendingAction_;

      typedef OneToOneQueue<toolbox::mem::Reference*> FragmentFIFO;
      FragmentFIFO fragmentFIFO_;

      FragmentGenerator fragmentGenerator_;

      PerformanceMonitor dataMonitoring_;
      mutable boost::mutex dataMonitoringMutex_;

      xdata::Double bandwidth_;
      xdata::Double frameRate_;
      xdata::Double fragmentRate_;
      xdata::Double fragmentSize_;
      xdata::Double fragmentSizeStdDev_;

      InfoSpaceItems dummyFerolParams_;
      xdata::String destinationClass_;
      xdata::UnsignedInteger32 destinationInstance_;
      xdata::Boolean usePlayback_;
      xdata::String playbackDataFile_;
      xdata::UnsignedInteger32 frameSize_;
      xdata::UnsignedInteger32 fedId_;
      xdata::UnsignedInteger32 fedSize_;
      xdata::UnsignedInteger32 fedSizeStdDev_;
      xdata::UnsignedInteger32 fragmentFIFOCapacity_;
    };


  } } //namespace evb::test

#endif // _evb_test_DummyFEROL_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
