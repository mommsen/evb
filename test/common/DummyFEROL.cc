#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/Exception.h"
#include "evb/version.h"
#include "evb/test/DummyFEROL.h"
#include "evb/test/dummyFEROL/States.h"
#include "evb/test/dummyFEROL/StateMachine.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <algorithm>
#include <pthread.h>

#define DOUBLE_WORKLOOPS

evb::test::DummyFEROL::DummyFEROL(xdaq::ApplicationStub* app) :
EvBApplication<dummyFEROL::Configuration,dummyFEROL::StateMachine>(app,"/evb/images/rui64x64.gif"),
doProcessing_(false),
fragmentFIFO_("fragmentFIFO")
{
  stateMachine_.reset( new dummyFEROL::StateMachine(this) );

  resetMonitoringCounters();
  startWorkLoops();

  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::test::DummyFEROL::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
}


void evb::test::DummyFEROL::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  bandwidth_ = 0;
  frameRate_ = 0;
  fragmentRate_ = 0;
  fragmentSize_ = 0;
  fragmentSizeStdDev_ = 0;

  monitoringParams.add("bandwidth", &bandwidth_);
  monitoringParams.add("frameRate", &frameRate_);
  monitoringParams.add("fragmentRate", &fragmentRate_);
  monitoringParams.add("fragmentSize", &fragmentSize_);
  monitoringParams.add("fragmentSizeStdDev", &fragmentSizeStdDev_);
}


void evb::test::DummyFEROL::do_updateMonitoringInfo()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  bandwidth_ = dataMonitoring_.bandwidth();
  frameRate_ = dataMonitoring_.i2oRate();
  fragmentRate_ = dataMonitoring_.logicalRate();
  fragmentSize_ = dataMonitoring_.size();
  fragmentSizeStdDev_ = dataMonitoring_.sizeStdDev();
  dataMonitoring_.reset();
}


void evb::test::DummyFEROL::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &evb::test::DummyFEROL::fragmentFIFOWebPage,
      "fragmentFIFO"
    );
}


void evb::test::DummyFEROL::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td class=\"component\">"                              << std::endl;
  *out << "<div>"                                                 << std::endl;
  *out << "<p>FEROL</p>"                                          << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);

    *out << "<tr>"                                                  << std::endl;
    *out << "<td>message count</td>"                                << std::endl;
    *out << "<td>" << dataMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->setf(std::ios::fixed);
    out->precision(2);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << bandwidth_ / 0x100000 << "</td>"              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (frame/s)</td>"                               << std::endl;
    *out << "<td>" << frameRate_ << "</td>"                         << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>rate (fragments/s)</td>"                           << std::endl;
    *out << "<td>" << fragmentRate_ << "</td>"                      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>FED size (kB)</td>"                                << std::endl;
    *out << "<td>" << fragmentSize_ / 0x400 << " +/- "
      << fragmentSizeStdDev_ / 0x400 << "</td>"                     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fragmentFIFO_.printHtml(out, getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void evb::test::DummyFEROL::fragmentFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "fragmentFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  fragmentFIFO_.printVerticalHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::test::DummyFEROL::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  dataMonitoring_.reset();
}


void evb::test::DummyFEROL::configure()
{
  clear();

  fragmentFIFO_.resize(configuration_->fragmentFIFOCapacity);

  fragmentGenerator_.configure(
    configuration_->fedId,
    configuration_->usePlayback,
    configuration_->playbackDataFile,
    configuration_->frameSize,
    configuration_->fedSize,
    configuration_->useLogNormal,
    configuration_->fedSizeStdDev,
    configuration_->minFedSize,
    configuration_->maxFedSize,
    configuration_->frameSize*configuration_->fragmentFIFOCapacity
  );

  getApplicationDescriptors();
}


void evb::test::DummyFEROL::getApplicationDescriptors()
{
  try
  {
    ruDescriptor_ =
      getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptor(configuration_->destinationClass,configuration_->destinationInstance);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;

    oss << "Failed to get application descriptor of the destination";

    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
}


void evb::test::DummyFEROL::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( fragmentFIFO_.deq(bufRef) ) { bufRef->release(); }
}


void evb::test::DummyFEROL::startProcessing()
{
  doProcessing_ = true;
  fragmentGenerator_.reset();
  generatingWL_->submit(generatingAction_);
  #ifdef DOUBLE_WORKLOOPS
  sendingWL_->submit(sendingAction_);
  #endif
}


void evb::test::DummyFEROL::stopProcessing()
{
  doProcessing_ = false;
  while (generatingActive_) ::usleep(1000);
  while (sendingActive_) ::usleep(1000);
}


void evb::test::DummyFEROL::startWorkLoops()
{
  try
  {
    generatingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( getIdentifier("generating"), "waiting" );

    if ( ! generatingWL_->isActive() )
    {
      generatingAction_ =
        toolbox::task::bind(this, &evb::test::DummyFEROL::generating,
          getIdentifier("generatingAction") );

      generatingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Generating'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }

  #ifdef DOUBLE_WORKLOOPS
  try
  {
    sendingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( getIdentifier("sending"), "waiting" );

    if ( ! sendingWL_->isActive() )
    {
      sendingAction_ =
        toolbox::task::bind(this, &evb::test::DummyFEROL::sending,
          getIdentifier("sendingAction") );

      sendingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Sending'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
  #endif
}


bool evb::test::DummyFEROL::generating(toolbox::task::WorkLoop *wl)
{
  generatingActive_ = true;

  toolbox::mem::Reference* bufRef = 0;

  try
  {
    while ( doProcessing_ )
    {
      if ( fragmentGenerator_.getData(bufRef) )
      {
        #ifdef DOUBLE_WORKLOOPS
        while ( doProcessing_ && !fragmentFIFO_.enq(bufRef) ) { ::usleep(1000); }
        #else
        updateCounters(bufRef);
        sendData(bufRef);
        #endif
      }
    }
  }
  catch(xcept::Exception &e)
  {
    generatingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  generatingActive_ = false;

  return doProcessing_;
}


bool __attribute__((optimize("O0")))  // Optimization causes segfaults as bufRef is null
evb::test::DummyFEROL::sending(toolbox::task::WorkLoop *wl)
{
  sendingActive_ = true;

  toolbox::mem::Reference* bufRef = 0;

  try
  {
    while ( doProcessing_ )
    {
      if ( fragmentFIFO_.deq(bufRef) )
      {
        updateCounters(bufRef);
        sendData(bufRef);
      }
      else
      {
        ::usleep(10);
      }
    }
  }
  catch(xcept::Exception &e)
  {
    sendingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  sendingActive_ = false;

  return doProcessing_;
}


inline void evb::test::DummyFEROL::updateCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

  const uint32_t payload = bufRef->getDataSize();

  ++dataMonitoring_.i2oCount;
  dataMonitoring_.logicalCount += configuration_->frameSize/(configuration_->fedSize+sizeof(ferolh_t));
  dataMonitoring_.sumOfSizes += payload;
  dataMonitoring_.sumOfSquares += payload*payload;
}


inline void evb::test::DummyFEROL::sendData(toolbox::mem::Reference* bufRef)
{
  getApplicationContext()->
    postFrame(
      bufRef,
      getApplicationDescriptor(),
      ruDescriptor_
    );
}


/**
 * Provides the factory method for the instantiation of DummyFEROL applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::test::DummyFEROL)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
