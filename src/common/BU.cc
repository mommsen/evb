#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/FUproxy.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/StateMachine.h"
#include "evb/InfoSpaceItems.h"
#include "evb/version.h"
#include "toolbox/task/WorkLoopFactory.h"

#include <math.h>


evb::BU::BU(xdaq::ApplicationStub *s) :
EvBApplication<bu::StateMachine>(s,evb::version,"/evb/images/bu64x64.gif"),
runNumber_(0),
doProcessing_(false),
processActive_(false)
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  diskWriter_.reset( new bu::DiskWriter(shared_from_this()) );
  ruProxy_.reset( new bu::RUproxy(this, fastCtrlMsgPool) );
  fuProxy_.reset( new bu::FUproxy(this, fastCtrlMsgPool) );
  eventTable_.reset( new bu::EventTable(shared_from_this(), diskWriter_, fuProxy_) );
  stateMachine_.reset( new bu::StateMachine(shared_from_this(),
      fuProxy_, ruProxy_, diskWriter_, eventTable_) );

  fuProxy_->registerEventTable(eventTable_);
  diskWriter_->registerEventTable(eventTable_);
  diskWriter_->registerStateMachine(stateMachine_);
  eventTable_->registerStateMachine(stateMachine_);
  
  initialize();
  
  resetMonitoringCounters();
  startProcessingWorkLoop();
  
  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::BU::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  nbEvtsUnderConstruction_ = 0;
  nbEventsInBU_ = 0;
  nbEvtsReady_ = 0;
  nbEvtsBuilt_ = 0;
  nbEvtsCorrupted_ = 0;
  nbFilesWritten_ = 0;
  
  appInfoSpaceParams.add("nbEvtsUnderConstruction", &nbEvtsUnderConstruction_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsInBU", &nbEventsInBU_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsReady", &nbEvtsReady_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsBuilt", &nbEvtsBuilt_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsCorrupted", &nbEvtsCorrupted_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbFilesWritten", &nbFilesWritten_, InfoSpaceItems::retrieve);

  ruProxy_->appendConfigurationItems(appInfoSpaceParams);
  fuProxy_->appendConfigurationItems(appInfoSpaceParams);
  diskWriter_->appendConfigurationItems(appInfoSpaceParams);
  eventTable_->appendConfigurationItems(appInfoSpaceParams); 
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void evb::BU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  deltaT_                     = 0;
  deltaN_                     = 0;
  deltaSumOfSquares_          = 0;
  deltaSumOfSizes_            = 0;
  
  monitoringParams.add("deltaT", &deltaT_);
  monitoringParams.add("deltaN", &deltaN_);
  monitoringParams.add("deltaSumOfSquares", &deltaSumOfSquares_);
  monitoringParams.add("deltaSumOfSizes", &deltaSumOfSizes_);

  ruProxy_->appendMonitoringItems(monitoringParams);
  fuProxy_->appendMonitoringItems(monitoringParams);
  diskWriter_->appendMonitoringItems(monitoringParams);
  eventTable_->appendMonitoringItems(monitoringParams); 
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::BU::do_updateMonitoringInfo()
{
  diskWriter_->updateMonitoringItems();
  eventTable_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
  fuProxy_->updateMonitoringItems();
  
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  
  PerformanceMonitor intervalEnd;
  eventTable_->getPerformance(intervalEnd);
  
  delta_ = intervalEnd - intervalStart_;
  
  deltaT_.value_ = delta_.seconds;
  deltaN_.value_ = delta_.N;
  deltaSumOfSizes_.value_ = delta_.sumOfSizes;
  deltaSumOfSquares_ = delta_.sumOfSquares;
  
  intervalStart_ = intervalEnd;
}


void evb::BU::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "nbEvtsUnderConstruction")
  {
    try
    {
      nbEvtsUnderConstruction_.setValue( *(monitoringInfoSpace_->find("nbEvtsUnderConstruction")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsUnderConstruction_ = 0;
    }
  }
  else if (item == "nbEventsInBU")
  {
    try
    {
      nbEventsInBU_.setValue( *(monitoringInfoSpace_->find("nbEventsInBU")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEventsInBU_ = 0;
    }
  }
  else if (item == "nbEvtsReady")
  {
    try
    {
      nbEvtsReady_.setValue( *(monitoringInfoSpace_->find("nbEvtsReady")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsReady_ = 0;
    }
  }
  else if (item == "nbEvtsBuilt")
  {
    try
    {
      nbEvtsBuilt_.setValue( *(monitoringInfoSpace_->find("nbEvtsBuilt")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsBuilt_ = 0;
    }
  }
  else if (item == "nbEvtsCorrupted")
  {
    try
    {
      nbEvtsCorrupted_.setValue( *(monitoringInfoSpace_->find("nbEvtsCorrupted")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsCorrupted_ = 0;
    }
  }
  else if (item == "nbFilesWritten")
  {
    try
    {
      nbFilesWritten_.setValue( *(monitoringInfoSpace_->find("nbFilesWritten")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbFilesWritten_ = 0;
    }
  }
}


void evb::BU::bindI2oCallbacks()
{
  i2o::bind
    (
      this,
      &evb::BU::I2O_BU_CACHE_Callback,
      I2O_BU_CACHE,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &evb::BU::I2O_BU_ALLOCATE_Callback,
      I2O_BU_ALLOCATE,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &evb::BU::I2O_BU_DISCARD_Callback,
      I2O_BU_DISCARD,
      XDAQ_ORGANIZATION_ID
    );

}


void evb::BU::I2O_BU_CACHE_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( bu::BuCache(bufRef) );
}


void evb::BU::I2O_BU_ALLOCATE_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( bu::BuAllocate(bufRef) );
}


void evb::BU::I2O_BU_DISCARD_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( bu::BuDiscard(bufRef) );
}


void evb::BU::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &evb::BU::completeEventsFIFOWebPage,
      "completeEventsFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::discardFIFOWebPage,
      "discardFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::freeResourceIdFIFOWebPage,
      "freeResourceIdFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::requestFIFOWebPage,
      "requestFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::blockFIFOWebPage,
      "blockFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::eventFIFOWebPage,
      "eventFIFO"
    );
  
  xgi::bind
    (
      this,
      &evb::BU::eolsFIFOWebPage,
      "eolsFIFO"
    );
}


void evb::BU::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  fuProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  ruProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  diskWriter_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void evb::BU::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BU</p>"                                             << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>run number</td>"                                   << std::endl;
  *out << "<td>" << runNumber_ << "</td>"                         << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  eventTable_->printMonitoringInformation(out);

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
    *out << "<td>event size (kB)</td>"                              << std::endl;
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
  
  eventTable_->printQueueInformation(out);
  
  eventTable_->printConfiguration(out);
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>maxEvtsUnderConstruction</td>"                     << std::endl;
  *out << "<td>" << stateMachine_->maxEvtsUnderConstruction() << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void evb::BU::completeEventsFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "completeEventsFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printCompleteEventsFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::discardFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "discardFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printDiscardFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::freeResourceIdFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "freeResourceIdFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printFreeResourceIdFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::eventFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "eventFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  diskWriter_->printEventFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::eolsFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "eolsFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  diskWriter_->printEoLSFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::requestFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "requestFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  fuProxy_->printRequestFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::blockFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "blockFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  ruProxy_->printBlockFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
 
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  intervalStart_ = PerformanceMonitor();
  delta_ = PerformanceMonitor();
}


void evb::BU::configure()
{}


void evb::BU::clear()
{}


void evb::BU::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void evb::BU::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


void evb::BU::startProcessingWorkLoop()
{
  try
  {
    const std::string identifier = getIdentifier();
    
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( getIdentifier("Processing"), "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &evb::BU::process,
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


bool evb::BU::process(toolbox::task::WorkLoop *wl)
{
  ::usleep(1000);

  processActive_ = true;
  
  try
  {
    while ( doProcessing_ && doWork() ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }        
  
  processActive_ = false;
  
  return doProcessing_;
}


bool evb::BU::doWork()
{
  bool anotherRound = false;
  //  toolbox::mem::Reference* bufRef;
  
  // // If there is an allocated event id
  // if( evmProxy_->getTriggerBlock(bufRef) )
  // {
  //   const uint32_t ruCount = ruProxy_->getRuCount();
  //   eventTable_->startConstruction(ruCount, bufRef);
    
  //   if ( ruCount > 0 )
  //     ruProxy_->requestDataForTrigger(bufRef);
    
  //   anotherRound = true;
  // }
  
  // // If there is an event data block
  // if( ruProxy_->getDataBlock(bufRef) )
  // {
  //   eventTable_->appendSuperFragment(bufRef);
    
  //   anotherRound = true;
  // }
  
  return anotherRound;
}


/**
 * Provides the factory method for the instantiation of RU applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::BU)


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
