#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/InfoSpaceItems.h"
#include "evb/PerformanceMonitor.h"
#include "evb/RU.h"
#include "evb/ru/BUproxy.h"
#include "evb/ru/Input.h"
#include "evb/ru/SuperFragmentTable.h"
#include "evb/TimerManager.h"
#include "evb/version.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::RU::RU(xdaq::ApplicationStub* s) :
EvBApplication<ru::StateMachine>(s,evb::version,"/evb/images/ru64x64.gif"),
doProcessing_(false),
processActive_(false),
timerId_(timerManager_.getTimer())
{
  superFragmentTable_.reset( new ru::SuperFragmentTable(shared_from_this()) );
  ruInput_.reset( new ru::Input(shared_from_this(), superFragmentTable_) );
  buProxy_.reset( new ru::BUproxy(shared_from_this(), superFragmentTable_) );    
  stateMachine_.reset( new ru::StateMachine(shared_from_this(), ruInput_, buProxy_) );

  buProxy_->registerStateMachine(stateMachine_);
  
  initialize();
  
  resetMonitoringCounters();
  startProcessingWorkLoop();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::RU::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  runNumber_ = 0;
  maxPairAgeMSec_ = 0; // Zero means forever
  
  i2oEVMRUDataReadyCount_ = 0;
  i2oBUCacheCount_ = 0;
  nbSuperFragmentsReady_ = 0;
  
  appInfoSpaceParams.add("runNumber", &runNumber_);
  appInfoSpaceParams.add("maxPairAgeMSec", &maxPairAgeMSec_);
  appInfoSpaceParams.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("i2oBUCacheCount", &i2oBUCacheCount_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_, InfoSpaceItems::retrieve);

  buProxy_->appendConfigurationItems(appInfoSpaceParams);
  ruInput_->appendConfigurationItems(appInfoSpaceParams);
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
  superFragmentTable_->appendConfigurationItems(appInfoSpaceParams);
}


void evb::RU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  monitoringRunNumber_        = 0;
  nbSuperFragmentsInRU_       = 0;

  deltaT_                     = 0;
  deltaN_                     = 0;
  deltaSumOfSquares_          = 0;
  deltaSumOfSizes_            = 0;
  
  monitoringParams.add("runNumber", &monitoringRunNumber_);
  monitoringParams.add("nbSuperFragmentsInRU", &nbSuperFragmentsInRU_);
  
  monitoringParams.add("deltaT", &deltaT_);
  monitoringParams.add("deltaN", &deltaN_);
  monitoringParams.add("deltaSumOfSquares", &deltaSumOfSquares_);
  monitoringParams.add("deltaSumOfSizes", &deltaSumOfSizes_);
  
  buProxy_->appendMonitoringItems(monitoringParams);
  ruInput_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
  superFragmentTable_->appendMonitoringItems(monitoringParams);
}


void evb::RU::do_updateMonitoringInfo()
{
  monitoringRunNumber_ = runNumber_;

  nbSuperFragmentsInRU_.value_ = std::max(static_cast<uint64_t>(0),
    ruInput_->fragmentsCount() - buProxy_->i2oBUCacheCount());
  
  buProxy_->updateMonitoringItems();
  ruInput_->updateMonitoringItems();
  stateMachine_->updateMonitoringItems();
  superFragmentTable_->updateMonitoringItems();

  boost::mutex::scoped_lock sl(performanceMonitorMutex_);

  PerformanceMonitor intervalEnd;
  getPerformance(intervalEnd);

  delta_ = intervalEnd - intervalStart_;
  
  deltaT_.value_ = delta_.seconds;
  deltaN_.value_ = delta_.N;
  deltaSumOfSizes_.value_ = delta_.sumOfSizes;
  deltaSumOfSquares_ = delta_.sumOfSquares;

  intervalStart_ = intervalEnd;
}


void evb::RU::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "inputSource")
  {
    ruInput_->inputSourceChanged();
  }
}


void evb::RU::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "i2oEVMRUDataReadyCount")
  {
    try
    {
      i2oEVMRUDataReadyCount_.setValue( *(monitoringInfoSpace_->find("i2oEVMRUDataReadyCount")) );
    }
    catch(xdata::exception::Exception& e)
    {
      i2oEVMRUDataReadyCount_ = 0;
    }
  }
  else if (item == "i2oBUCacheCount")
  {
    try
    {
      i2oBUCacheCount_.setValue( *(monitoringInfoSpace_->find("i2oBUCacheCount")) );
    }
    catch(xdata::exception::Exception& e)
    {
      i2oBUCacheCount_ = 0;
    }
  }
  else if (item == "nbSuperFragmentsReady")
  {
    try
    {
      nbSuperFragmentsReady_.setValue( *(monitoringInfoSpace_->find("nbSuperFragmentsReady")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbSuperFragmentsReady_ = 0;
    }
  }
}


void evb::RU::bindI2oCallbacks()
{  
  i2o::bind
    (
      this,
      &evb::RU::I2O_EVMRU_DATA_READY_Callback,
      I2O_EVMRU_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &evb::RU::I2O_RU_SEND_Callback,
      I2O_RU_SEND,
      XDAQ_ORGANIZATION_ID
    );
}


void evb::RU::I2O_EVMRU_DATA_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( ru::EvmRuDataReady(bufRef) );
}


void evb::RU::I2O_RU_SEND_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( ru::RuSend(bufRef) );
}


void evb::RU::bindNonDefaultXgiCallbacks()
{
}


void evb::RU::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  ruInput_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  buProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void evb::RU::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RU</p>"                                            << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>run number</td>"                                   << std::endl;
  *out << "<td>" << runNumber_ << "</td>"                         << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># fragments in RU</td>"                            << std::endl;
  *out << "<td>" << nbSuperFragmentsInRU_ << "</td>"              << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># fragments built</td>"                            << std::endl;
    *out << "<td>" << superFragmentMonitoring_.count << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
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
      (delta_.seconds>0 ? delta_.sumOfSizes/(double)0x100000/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.N/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>fragment size (kB)</td>"                           << std::endl;
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
  *out << "<th colspan=\"2\"><br/>Configuration</th>"             << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>monitoringSleepSec</td>"                           << std::endl;
  *out << "<td>" << monitoringSleepSec_ << "</td>"                << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>maxPairAgeMSec</td>"                               << std::endl;
  *out << "<td>" << maxPairAgeMSec_ << "</td>"                    << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void evb::RU::getPerformance(PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);

  performanceMonitor.N = superFragmentMonitoring_.count;
  performanceMonitor.sumOfSizes = superFragmentMonitoring_.payload;
  performanceMonitor.sumOfSquares = superFragmentMonitoring_.payloadSquared;
}


void evb::RU::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(performanceMonitorMutex_);
    intervalStart_ = PerformanceMonitor();
    delta_ = PerformanceMonitor();
  }
  {
    boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
    superFragmentMonitoring_.count = 0;
    superFragmentMonitoring_.payload = 0;
    superFragmentMonitoring_.payloadSquared = 0;
  }
}


void evb::RU::configure()
{
  timerManager_.initTimer(timerId_, maxPairAgeMSec_);
}


void evb::RU::clear()
{
  superFragmentTable_->clear();
}


void evb::RU::startProcessing()
{
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void evb::RU::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


void evb::RU::startProcessingWorkLoop()
{
  try
  {
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( getIdentifier("Processing"), "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &evb::RU::process,
          getIdentifier("process") );
      
      processingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Processing'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::RU::process(toolbox::task::WorkLoop *wl)
{
  processActive_ = true;
  
  try
  {
    // // Wait for a trigger
    // EvBid evbId;
    // while ( doProcessing_ && ! evmProxy_->getTrigEvBid(evbId) ) ::usleep(1000);
    
    // // Wait for the corresponding event fragment
    // timerManager_.restartTimer(timerId_);
    // toolbox::mem::Reference* bufRef = 0;
    // while ( doProcessing_ && ! ruInput_->getData(evbId,bufRef) )
    // {
    //   ::usleep(1000);
    //   if ( maxPairAgeMSec_ > 0 && timerManager_.isFired(timerId_) )
    //   {
    //     std::ostringstream msg;
    //     msg << "Waited for more than " << maxPairAgeMSec_ << 
    //       " ms for the event data block for trigger evbId " << evbId;
    //     XCEPT_RAISE(exception::TimedOut, msg.str());
    //   }
    // }
    
    // if (bufRef)
    // {
    //   updateSuperFragmentCounters(bufRef);
    //   superFragmentTable_->addEvBidAndBlock(evbId, bufRef);
    // }
  }
  catch(exception::MismatchDetected &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( ru::MismatchDetected(e) );
  }
  catch(exception::TimedOut &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( ru::TimedOut(e) );
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }

  processActive_ = false;

  return doProcessing_;
}


void evb::RU::updateSuperFragmentCounters(toolbox::mem::Reference* head)
{
  uint32_t payload = 0;
  toolbox::mem::Reference* bufRef = head;

  while (bufRef)
  {
    const I2O_MESSAGE_FRAME* stdMsg =
      (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    payload +=
      (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
    bufRef = bufRef->getNextReference();
  }

  boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
  
  ++superFragmentMonitoring_.count;
  superFragmentMonitoring_.payload += payload;
  superFragmentMonitoring_.payloadSquared += payload*payload;
}


/**
 * Provides the factory method for the instantiation of RU applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::RU)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
