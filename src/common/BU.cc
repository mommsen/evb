#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/BU.h"
#include "evb/bu/DiskWriter.h"
#include "evb/bu/EventBuilder.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/RUproxy.h"
#include "evb/bu/StateMachine.h"
#include "evb/InfoSpaceItems.h"
#include "toolbox/task/WorkLoopFactory.h"

#include <math.h>


evb::BU::BU(xdaq::ApplicationStub *app) :
EvBApplication<bu::Configuration,bu::StateMachine>(app,"/evb/images/bu64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  resourceManager_.reset( new bu::ResourceManager(this) );
  diskWriter_.reset( new bu::DiskWriter(this, resourceManager_) );
  eventBuilder_.reset( new bu::EventBuilder(this, diskWriter_, resourceManager_) );
  ruProxy_.reset( new bu::RUproxy(this, eventBuilder_, resourceManager_, fastCtrlMsgPool) );
  stateMachine_.reset( new bu::StateMachine(this,
      ruProxy_, diskWriter_, eventBuilder_, resourceManager_) );

  diskWriter_->registerRUproxy(ruProxy_);
  diskWriter_->registerStateMachine(stateMachine_);
  eventBuilder_->registerStateMachine(stateMachine_);
  ruProxy_->registerStateMachine(stateMachine_);

  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void evb::BU::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  eventSize_ = 0;
  eventRate_ = 0;
  nbEventsInBU_ = 0;
  nbEventsBuilt_ = 0;
  nbLumiSections_ = 0;

  appInfoSpaceParams.add("eventSize", &eventSize_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("eventRate", &eventRate_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsInBU", &nbEventsInBU_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsBuilt", &nbEventsBuilt_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbLumiSections", &nbLumiSections_, InfoSpaceItems::retrieve);
}


void evb::BU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  diskWriter_->appendMonitoringItems(monitoringParams);
  resourceManager_->appendMonitoringItems(monitoringParams);
  ruProxy_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::BU::do_updateMonitoringInfo()
{
  diskWriter_->updateMonitoringItems();
  resourceManager_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
  stateMachine_->updateMonitoringItems();
}


void evb::BU::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "eventSize")
  {
    try
    {
      eventSize_.setValue( *(monitoringInfoSpace_->find("eventSize")) );
    }
    catch(xdata::exception::Exception& e)
    {
      eventSize_ = 0;
    }
  }
  else if (item == "eventRate")
  {
    try
    {
      eventRate_.setValue( *(monitoringInfoSpace_->find("eventRate")) );
    }
    catch(xdata::exception::Exception& e)
    {
      eventRate_ = 0;
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
  else if (item == "nbEventsBuilt")
  {
    try
    {
      nbEventsBuilt_.setValue( *(monitoringInfoSpace_->find("nbEventsBuilt")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEventsBuilt_ = 0;
    }
  }
  else if (item == "nbLumiSections")
  {
    try
    {
      nbLumiSections_.setValue( *(monitoringInfoSpace_->find("nbLumiSections")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbLumiSections_ = 0;
    }
  }
}


void evb::BU::do_bindI2oCallbacks()
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
      &evb::BU::freeResourceFIFOWebPage,
      "freeResourceFIFO"
    );

  xgi::bind
    (
      this,
      &evb::BU::blockedResourceFIFOWebPage,
      "blockedResourceFIFO"
    );

  xgi::bind
    (
      this,
      &evb::BU::lumiSectionAccountFIFOWebPage,
      "lumiSectionAccountFIFO"
    );
}


void evb::BU::do_defaultWebPage
(
  xgi::Output *out
) const
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\" rowspan=\"3\">"              << std::endl;
  ruProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  resourceManager_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_w.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\" rowspan=\"3\">"              << std::endl;
  diskWriter_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td></td>"                                           << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_ns.gif\" alt=\"\"/>"    << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td></td>"                                           << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  eventBuilder_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void evb::BU::freeResourceFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "freeResourceFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  resourceManager_->printFreeResourceFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::blockedResourceFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "blockedResourceFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  resourceManager_->printBlockedResourceFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void evb::BU::lumiSectionAccountFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "lumiSectionAccountFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  resourceManager_->printLumiSectionAccountFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
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
