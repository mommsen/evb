#include "i2o/Method.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/InfoSpaceItems.h"
#include "evb/PerformanceMonitor.h"
#include "evb/RU.h"
#include "evb/ru/BUproxy.h"
#include "evb/ru/Input.h"
#include "evb/TimerManager.h"
#include "evb/version.h"
#include "pt/utcp/frl/Method.h"
#include "toolbox/task/WorkLoopFactory.h"


evb::RU::RU(xdaq::ApplicationStub* s) :
EvBApplication<ru::StateMachine>(s,evb::version,"/evb/images/ru64x64.gif")
{
  ruInput_.reset( new ru::Input(this) );
  buProxy_.reset( new ru::BUproxy(this, ruInput_) );    
  stateMachine_.reset( new ru::StateMachine(this, ruInput_, buProxy_) );

  buProxy_->registerStateMachine(stateMachine_);
  
  initialize();
  
  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::RU::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  nbSuperFragmentsReady_ = 0;
  
  appInfoSpaceParams.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_, InfoSpaceItems::retrieve);
  
  buProxy_->appendConfigurationItems(appInfoSpaceParams);
  ruInput_->appendConfigurationItems(appInfoSpaceParams);
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void evb::RU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  buProxy_->appendMonitoringItems(monitoringParams);
  ruInput_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::RU::do_updateMonitoringInfo()
{
  buProxy_->updateMonitoringItems();
  ruInput_->updateMonitoringItems();
  stateMachine_->updateMonitoringItems();
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
  if (item == "nbSuperFragmentsReady")
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
      &evb::RU::I2O_DATA_READY_Callback,
      I2O_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &evb::RU::I2O_RU_SEND_Callback,
      I2O_SHIP_FRAGMENTS,
      XDAQ_ORGANIZATION_ID
    );
  
  pt::utcp::frl::bind
    (
      this,
      &evb::RU::rawDataAvailable
    );
}


void evb::RU::I2O_DATA_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  //ruInput_->dataReadyCallback(bufRef);
}


void evb::RU::I2O_RU_SEND_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  buProxy_->rqstForFragmentsMsgCallback(bufRef);
}


void evb::RU::rawDataAvailable
(
  toolbox::mem::Reference* bufRef,
  int originator,
  pt::utcp::frl::MemoryCache* cache
)
{
  ruInput_->dataReadyCallback(bufRef,cache);
}


void evb::RU::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &evb::RU::requestFIFOWebPage,
      "requestFIFO"
    );
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
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  buProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void evb::RU::requestFIFOWebPage
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
  buProxy_->printRequestFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::RU::configure()
{}


void evb::RU::clear()
{}


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
