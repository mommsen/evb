#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/Exception.h"
#include "evb/version.h"
#include "evb/test/DummyFEROL.h"
#include "evb/test/dummyFEROL/StateMachine.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <algorithm>
#include <pthread.h>

#define DOUBLE_WORKLOOPS

evb::test::DummyFEROL::DummyFEROL(xdaq::ApplicationStub* s) :
EvBApplication<dummyFEROL::StateMachine>(s,evb::version,"/evb/images/rui64x64.gif"),
doProcessing_(false),
fragmentFIFO_("fragmentFIFO")
{
  stateMachine_.reset( new dummyFEROL::StateMachine(this) );
  
  initialize();
  
  resetMonitoringCounters();
  startWorkLoops();
  
  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::test::DummyFEROL::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  destinationClass_ = "rubuilder::ru::Application";
  destinationInstance_ = instance_;
  fedId_ = instance_;
  fedSize_ = 2048;
  fedSizeStdDev_ = 0;
  usePlayback_ = false;
  playbackDataFile_ = "";  
  frameSize_ = 0x40000;
  fragmentFIFOCapacity_ = 32;
  i2oDataReadyCount_ = 0;

  dummyFerolParams_.add("destinationClass", &destinationClass_);
  dummyFerolParams_.add("destinationInstance", &destinationInstance_);
  dummyFerolParams_.add("fedId", &fedId_);
  dummyFerolParams_.add("fedSize", &fedSize_);
  dummyFerolParams_.add("fedSizeStdDev", &fedSizeStdDev_);
  dummyFerolParams_.add("usePlayback", &usePlayback_);
  dummyFerolParams_.add("playbackDataFile", &playbackDataFile_);
  dummyFerolParams_.add("frameSize", &frameSize_);
  dummyFerolParams_.add("fragmentFIFOCapacity", &fragmentFIFOCapacity_);

  stateMachine_->appendConfigurationItems(dummyFerolParams_);

  appInfoSpaceParams.add(dummyFerolParams_);
  appInfoSpaceParams.add("i2oDataReadyCount", &i2oDataReadyCount_, InfoSpaceItems::retrieve);
}


void evb::test::DummyFEROL::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  i2oDataReadyCount_ = 0;
  
  monitoringParams.add("i2oDataReadyCount", &i2oDataReadyCount_);
  
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::test::DummyFEROL::do_updateMonitoringInfo()
{
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  
  PerformanceMonitor intervalEnd;
  getPerformance(intervalEnd);
  delta_ = intervalEnd - intervalStart_;
  intervalStart_ = intervalEnd;

  i2oDataReadyCount_ = intervalEnd.N;
}


void evb::test::DummyFEROL::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "i2oDataReadyCount")
  {
    try
    {
      i2oDataReadyCount_.setValue( *(monitoringInfoSpace_->find("i2oDataReadyCount")) );
    }
    catch(xdata::exception::Exception& e)
    {
      i2oDataReadyCount_ = 0;
    }
  }
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
    *out << "<td>last evt number to RU</td>"                        << std::endl;
    *out << "<td>" << dataMonitoring_.lastEventNumberToRU << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Sent to RU</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << dataMonitoring_.payload / 0x100000<< "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>message count</td>"                                << std::endl;
    *out << "<td>" << dataMonitoring_.msgCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>"                 << std::endl;
  
  {
    boost::mutex::scoped_lock sl(performanceMonitorMutex_);
    
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->precision(3);
    out->setf(std::ios::fixed);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>deltaT (s)</td>"                                   << std::endl;
    *out << "<td>" << delta_.seconds << "</td>"                     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->precision(2);
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? static_cast<double>(delta_.sumOfSizes)/0x100000/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (frame/s)</td>"                              << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.N/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>rate (fragments/s)</td>"                           << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.N/delta_.seconds*(frameSize_/(fedSize_+16)) : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>frame size (kB)</td>"                           << std::endl;
    *out << "<td>";
    if ( delta_.N>0 )
    {
      const double meanOfSquares =  static_cast<double>(delta_.sumOfSquares)/delta_.N;
      const double mean = static_cast<double>(delta_.sumOfSizes)/delta_.N;
      const double variance = meanOfSquares - (mean*mean);
      // Variance maybe negative due to lack of precision
      const double rms = variance > 0 ? std::sqrt(variance) : 0;
      *out << mean/0x400 << " +/- " << rms/0x400;
    }
    else
    {
      *out << "n/a";
    }
    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }
 
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fragmentFIFO_.printHtml(out, getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  dummyFerolParams_.printHtml("Configuration", out);

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


void evb::test::DummyFEROL::getPerformance(PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

  performanceMonitor.N = dataMonitoring_.msgCount;
  performanceMonitor.sumOfSizes = dataMonitoring_.payload;
  performanceMonitor.sumOfSquares = dataMonitoring_.payloadSquared;
}


void evb::test::DummyFEROL::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  
  dataMonitoring_.lastEventNumberToRU = 0;
  dataMonitoring_.msgCount = 0;
  dataMonitoring_.payload = 0;
  dataMonitoring_.payloadSquared = 0;
}


void evb::test::DummyFEROL::configure()
{
  clear();

  fragmentFIFO_.resize(fragmentFIFOCapacity_);

  fragmentGenerator_.configure(
    fedId_, usePlayback_, playbackDataFile_,
    frameSize_, fedSize_, fedSizeStdDev_,
    frameSize_*fragmentFIFOCapacity_
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
      getApplicationDescriptor(destinationClass_,destinationInstance_);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor of RU";
    oss << destinationClass_.toString() << destinationInstance_.toString();
    
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
      getWorkLoop( getIdentifier("Generating"), "waiting" );
    
    if ( ! generatingWL_->isActive() )
    {
      generatingAction_ =
        toolbox::task::bind(this, &evb::test::DummyFEROL::generating,
          getIdentifier("generating") );
      
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
      getWorkLoop( getIdentifier("Sending"), "waiting" );
    
    if ( ! sendingWL_->isActive() )
    {
      sendingAction_ =
        toolbox::task::bind(this, &evb::test::DummyFEROL::sending,
          getIdentifier("sending") );
      
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
  
  //fix affinity to core 0
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  const int status = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if ( status != 0  )
  {
    std::ostringstream oss;
    oss << "Failed to set affinity for workloop 'Generating': "
      << strerror(status);
    XCEPT_RAISE(exception::DummyData, oss.str());
  }

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
  
  generatingActive_ = false;
  
  return doProcessing_;
}


bool evb::test::DummyFEROL::sending(toolbox::task::WorkLoop *wl)
{
  sendingActive_ = true;

  toolbox::mem::Reference* bufRef = 0;

  //fix affinity to core 1
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  const int status = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if ( status != 0  )
  {
    std::ostringstream oss;
    oss << "Failed to set affinity for workloop 'Sending': "
      << strerror(status);
    XCEPT_RAISE(exception::DummyData, oss.str());
  }

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
  
  sendingActive_ = false;
  
  return doProcessing_;
}


inline void evb::test::DummyFEROL::updateCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  
  const uint32_t payload = bufRef->getDataSize();
  
  ++dataMonitoring_.msgCount;
  dataMonitoring_.payload += payload;
  dataMonitoring_.payloadSquared += payload*payload;
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
