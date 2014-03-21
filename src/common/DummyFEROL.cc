#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/DummyFEROL.h"
#include "evb/dummyFEROL/States.h"
#include "evb/dummyFEROL/StateMachine.h"
#include "evb/Exception.h"
#include "evb/version.h"
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

  LOG4CPLUS_INFO(getApplicationLogger(), "End of constructor");
}


void evb::test::DummyFEROL::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  stopAtEvent_ = 0;
  skipNbEvents_ = 0;
  duplicateNbEvents_ = 0;

  appInfoSpaceParams.add("lastEventNumber", &lastEventNumber_);
  appInfoSpaceParams.add("stopAtEvent", &stopAtEvent_);
  appInfoSpaceParams.add("skipNbEvents", &skipNbEvents_);
  appInfoSpaceParams.add("duplicateNbEvents", &duplicateNbEvents_);
}


void evb::test::DummyFEROL::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  lastEventNumber_ = 0;
  bandwidth_ = 0;
  frameRate_ = 0;
  fragmentRate_ = 0;
  fragmentSize_ = 0;
  fragmentSizeStdDev_ = 0;

  monitoringParams.add("lastEventNumber", &lastEventNumber_);
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
) const
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
    *out << "<td>last event number</td>"                            << std::endl;
    *out << "<td>" << lastEventNumber_.value_ << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->setf(std::ios::fixed);
    out->precision(2);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << bandwidth_.value_ / 0x100000 << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (frame/s)</td>"                               << std::endl;
    *out << "<td>" << frameRate_.value_ << "</td>"                  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>rate (fragments/s)</td>"                           << std::endl;
    *out << "<td>" << fragmentRate_.value_ << "</td>"               << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>FED size (kB)</td>"                                << std::endl;
    *out << "<td>" << fragmentSize_.value_ / 0x400 << " +/- "
      << fragmentSizeStdDev_.value_ / 0x400 << "</td>"              << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fragmentFIFO_.printHtml(out, urn_);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
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
  lastEventNumber_ = 0;
}


void evb::test::DummyFEROL::configure()
{
  fragmentFIFO_.clear();
  fragmentFIFO_.resize(configuration_->fragmentFIFOCapacity);

  fragmentGenerator_.configure(
    configuration_->fedId,
    configuration_->usePlayback,
    configuration_->playbackDataFile,
    configuration_->frameSize,
    configuration_->fedSize,
    configuration_->computeCRC,
    configuration_->useLogNormal,
    configuration_->fedSizeStdDev,
    configuration_->minFedSize,
    configuration_->maxFedSize,
    configuration_->frameSize*configuration_->fragmentFIFOCapacity,
    configuration_->fakeLumiSectionDuration
  );

  getApplicationDescriptors();

  resetMonitoringCounters();
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
    std::ostringstream oss;
    oss << "Failed to get application descriptor of the destination";
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
}


void evb::test::DummyFEROL::startProcessing()
{
  resetMonitoringCounters();
  doProcessing_ = true;
  fragmentGenerator_.reset();
  generatingWL_->submit(generatingAction_);
  #ifdef DOUBLE_WORKLOOPS
  sendingWL_->submit(sendingAction_);
  #endif
}


void evb::test::DummyFEROL::drain()
{
  if ( stopAtEvent_.value_ == 0 ) stopAtEvent_.value_ = lastEventNumber_;
  while ( generatingActive_ || !fragmentFIFO_.empty() || sendingActive_ ) ::usleep(1000);
}


void evb::test::DummyFEROL::stopProcessing()
{
  doProcessing_ = false;
  while ( generatingActive_ || sendingActive_ ) ::usleep(1000);
  fragmentFIFO_.clear();
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
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Generating'";
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
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Sending'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
  #endif
}


bool evb::test::DummyFEROL::generating(toolbox::task::WorkLoop *wl)
{
  generatingActive_ = true;

  try
  {
    while ( doProcessing_ )
    {
      toolbox::mem::Reference* bufRef = 0;

      if ( fragmentGenerator_.getData(bufRef) )
      {
        if (skipNbEvents_.value_ > 0)
        {
          bufRef->release();
          --skipNbEvents_;
        }
        else
        {
          lastEventNumber_ = fragmentGenerator_.getLastEventNumber();
          if (stopAtEvent_.value_ == 0 || lastEventNumber_ < stopAtEvent_.value_)
          {
            do
            {
              if ( duplicateNbEvents_.value_ > 0 )
                bufRef->duplicate();

              #ifdef DOUBLE_WORKLOOPS
              fragmentFIFO_.enqWait(bufRef);
              #else
              updateCounters(bufRef);
              sendData(bufRef);
              #endif
            }
            while ( duplicateNbEvents_-- > 0 );
          }
          else
          {
            bufRef->release();
            doProcessing_ = false;
          }
        }
      }
    }
  }
  catch(xcept::Exception &e)
  {
    generatingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    generatingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
      sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    generatingActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
      sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  generatingActive_ = false;

  return doProcessing_;
}


bool __attribute__((optimize("O0")))  // Optimization causes segfaults as bufRef is null
evb::test::DummyFEROL::sending(toolbox::task::WorkLoop *wl)
{
  sendingActive_ = true;

  try
  {
    toolbox::mem::Reference* bufRef = 0;

    while ( fragmentFIFO_.deq(bufRef) )
    {
      updateCounters(bufRef);
      sendData(bufRef);
    }
  }
  catch(xcept::Exception &e)
  {
    sendingActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  sendingActive_ = false;

  ::usleep(10);

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
