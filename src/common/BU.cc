#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/EventTable.h"
#include "evb/bu/StateMachine.h"
#include "evb/InfoSpaceItems.h"
#include "evb/version.h"
#include "toolbox/task/WorkLoopFactory.h"

#include <math.h>


evb::BU::BU(xdaq::ApplicationStub *s) :
EvBApplication<bu::StateMachine>(s,evb::version,"/evb/images/bu64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  diskWriter_.reset( new bu::DiskWriter(this) );
  ruProxy_.reset( new bu::RUproxy(this, fastCtrlMsgPool) );
  eventTable_.reset( new bu::EventTable(this, ruProxy_, diskWriter_) );
  stateMachine_.reset( new bu::StateMachine(this,
      ruProxy_, diskWriter_, eventTable_) );

  diskWriter_->registerEventTable(eventTable_);
  diskWriter_->registerStateMachine(stateMachine_);
  eventTable_->registerStateMachine(stateMachine_);
  
  initialize();
  
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
  diskWriter_->appendConfigurationItems(appInfoSpaceParams);
  eventTable_->appendConfigurationItems(appInfoSpaceParams); 
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void evb::BU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  ruProxy_->appendMonitoringItems(monitoringParams);
  diskWriter_->appendMonitoringItems(monitoringParams);
  eventTable_->appendMonitoringItems(monitoringParams); 
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::BU::do_updateMonitoringInfo()
{
  diskWriter_->updateMonitoringItems();
  eventTable_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
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
}


void evb::BU::I2O_BU_CACHE_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  ruProxy_->superFragmentCallback(bufRef);
}


void evb::BU::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &evb::BU::fragmentFIFOWebPage,
      "fragmentFIFO"
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
  *out << "<td class=\"component\">"                            << std::endl;
  ruProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  eventTable_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  diskWriter_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
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


void evb::BU::fragmentFIFOWebPage
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
  ruProxy_->printFragmentFIFO(out);
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
  eventTable_->printBlockFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
 
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::configure()
{}


void evb::BU::clear()
{}


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
