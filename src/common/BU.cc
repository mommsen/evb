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
#include "xgi/Method.h"
#include "xgi/Utils.h"

#include <math.h>


evb::BU::BU(xdaq::ApplicationStub* app) :
  EvBApplication<bu::Configuration,bu::StateMachine>(app,"/evb/images/bu64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  resourceManager_.reset( new bu::ResourceManager(this) );
  diskWriter_.reset( new bu::DiskWriter(this, resourceManager_) );
  eventBuilder_.reset( new bu::EventBuilder(this, diskWriter_, resourceManager_) );
  ruProxy_.reset( new bu::RUproxy(this, eventBuilder_, resourceManager_, fastCtrlMsgPool) );
  stateMachine_.reset( new bu::StateMachine(this,
                                            ruProxy_, diskWriter_, eventBuilder_, resourceManager_) );

  diskWriter_->registerStateMachine(stateMachine_);
  eventBuilder_->registerStateMachine(stateMachine_);
  ruProxy_->registerStateMachine(stateMachine_);

  initialize();

  LOG4CPLUS_INFO(getApplicationLogger(), "End of constructor");
}


void evb::BU::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  eventSize_ = 0;
  eventRate_ = 0;
  throughput_ = 0;
  nbEventsInBU_ = 0;
  nbEventsBuilt_ = 0;
  nbLumiSections_ = 0;
  nbCorruptedEvents_ = 0;
  nbEventsWithCRCerrors_ = 0;
  nbEventsMissingData_ = 0;

  appInfoSpaceParams.add("eventSize", &eventSize_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("eventRate", &eventRate_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("throughput", &throughput_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsInBU", &nbEventsInBU_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsBuilt", &nbEventsBuilt_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbLumiSections", &nbLumiSections_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbCorruptedEvents", &nbCorruptedEvents_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsWithCRCerrors", &nbEventsWithCRCerrors_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsMissingData", &nbEventsMissingData_, InfoSpaceItems::retrieve);
}


void evb::BU::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  diskWriter_->appendMonitoringItems(monitoringParams);
  resourceManager_->appendMonitoringItems(monitoringParams);
  eventBuilder_->appendMonitoringItems(monitoringParams);
  ruProxy_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void evb::BU::do_updateMonitoringInfo()
{
  diskWriter_->updateMonitoringItems();
  resourceManager_->updateMonitoringItems();
  eventBuilder_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
  stateMachine_->updateMonitoringItems();
}


void evb::BU::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "eventSize")
    eventSize_ = resourceManager_->getEventSize();
  else if (item == "eventRate")
    eventRate_ = resourceManager_->getEventRate();
  else if (item == "throughput")
    throughput_ = resourceManager_->getThroughput();
  else if (item == "nbEventsInBU")
    nbEventsInBU_ = resourceManager_->getNbEventsInBU();
  else if (item == "nbEventsBuilt")
    nbEventsBuilt_ = resourceManager_->getNbEventsBuilt();
  else if (item == "nbLumiSections")
    nbLumiSections_ = diskWriter_->getNbLumiSections();
  else if (item == "nbCorruptedEvents")
    nbCorruptedEvents_ = eventBuilder_->getNbCorruptedEvents();
  else if (item == "nbEventsWithCRCerrors")
    nbEventsWithCRCerrors_ = eventBuilder_->getNbEventsWithCRCerrors();
  else if (item == "nbEventsMissingData")
    nbEventsMissingData_ = eventBuilder_->getNbEventsMissingData();
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
  toolbox::mem::Reference* bufRef
)
throw (i2o::exception::Exception)
{
  try
  {
    ruProxy_->superFragmentCallback(bufRef);
  }
  catch(xcept::Exception& e)
  {
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    this->stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, "unkown exception");
    this->stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


void evb::BU::bindNonDefaultXgiCallbacks()
{
  xgi::framework::deferredbind(this, this,
                               &evb::BU::displayResourceTable,
                               "resourceTable");

  xgi::bind(
    this,
    &evb::BU::writeNextEventsToFile,
    "writeNextEventsToFile"
  );
}


cgicc::table evb::BU::getMainWebPage() const
{
  using namespace cgicc;

  table layoutTable;
  layoutTable.set("class","xdaq-evb-layout");
  layoutTable.add(colgroup()
                  .add(col())
                  .add(col().set("class","xdaq-evb-arrow"))
                  .add(col())
                  .add(col().set("class","xdaq-evb-arrow"))
                  .add(col()));
  layoutTable.add(tr()
                  .add(td(this->getWebPageBanner()).set("colspan","5")));

  layoutTable.add(tr()
                  .add(td(ruProxy_->getHtmlSnipped()).set("class","xdaq-evb-component").set("rowspan","3"))
                  .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
                  .add(td(resourceManager_->getHtmlSnipped()).set("class","xdaq-evb-component"))
                  .add(td(img().set("src","/evb/images/arrow_w.gif").set("alt","")))
                  .add(td(diskWriter_->getHtmlSnipped()).set("class","xdaq-evb-component").set("rowspan","3")));

  layoutTable.add(tr()
                  .add(td(" "))
                  .add(td(img().set("src","/evb/images/arrow_ns.gif").set("alt","")))
                  .add(td(" ")));

  layoutTable.add(tr()
                  .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
                  .add(td(eventBuilder_->getHtmlSnipped()).set("class","xdaq-evb-component"))
                  .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt",""))));

  return layoutTable;
}


void evb::BU::displayResourceTable(xgi::Input* in,xgi::Output* out)
throw (xgi::exception::Exception)
{
  using namespace cgicc;
  table layoutTable;

  layoutTable.set("class","xdaq-evb-layout");
  layoutTable.add(tr()
                  .add(td(getWebPageBanner())));
  layoutTable.add(tr()
                  .add(td(resourceManager_->getHtmlSnippedForResourceTable())));

  *out << getWebPageHeader();
  *out << layoutTable;
}


void evb::BU::writeNextEventsToFile(xgi::Input* in,xgi::Output* out)
throw (xgi::exception::Exception)
{
  cgicc::Cgicc cgi(in);
  uint16_t count = 1;

  if ( xgi::Utils::hasFormElement(cgi,"count") )
    count = xgi::Utils::getFormElement(cgi, "count")->getIntegerValue();

  eventBuilder_->writeNextEventsToFile(count);
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
