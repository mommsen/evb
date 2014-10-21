#ifndef _evb_readoutunit_ReadoutUnit_h_
#define _evb_readoutunit_ReadoutUnit_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "cgicc/HTMLClasses.h"
#include "evb/EvBApplication.h"
#include "evb/InfoSpaceItems.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/Input.h"
#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "interface/shared/i2ogevb2g.h"
#include "pt/frl/Method.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Input.h"
#include "xgi/Method.h"
#include "xgi/Output.h"
#include "xgi/Utils.h"


namespace evb {

  namespace readoutunit {

    class Configuration;

    /**
     * \ingroup xdaqApps
     * \brief Template for the readout units (EVM/RU)
     */
    template<class Unit,class Configuration,class StateMachine>
    class ReadoutUnit : public EvBApplication<Configuration,StateMachine>
    {
    public:

      ReadoutUnit
      (
        xdaq::ApplicationStub*,
        const std::string& appIcon
      );

      typedef boost::shared_ptr< Input<Unit,Configuration> > InputPtr;
      InputPtr getInput() const
      { return input_; }

      typedef boost::shared_ptr< BUproxy<Unit> > BUproxyPtr;
      BUproxyPtr getBUproxy() const
      { return buProxy_; }

      uint32_t getEventNumberToStop() const
      { return stopLocalInputAtEvent_.value_; }


    protected:

      InputPtr input_;
      BUproxyPtr buProxy_;

    private:

      virtual void do_bindI2oCallbacks();
      inline void rawDataAvailable(toolbox::mem::Reference*, int originator, tcpla::MemoryCache*) throw (pt::frl::exception::Exception);
      inline void I2O_SHIP_FRAGMENTS_Callback(toolbox::mem::Reference*) throw (i2o::exception::Exception);

      virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
      virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
      virtual void do_updateMonitoringInfo();

      virtual void do_handleItemChangedEvent(const std::string& item);
      virtual void do_handleItemRetrieveEvent(const std::string& item);

      virtual void bindNonDefaultXgiCallbacks();
      virtual cgicc::table getMainWebPage() const;
      virtual void addComponentsToWebPage(cgicc::table&) const;

      void eventCountForLumiSection(xgi::Input*, xgi::Output*) throw (xgi::exception::Exception);
      void writeNextFragmentsToFile(xgi::Input*, xgi::Output*) throw (xgi::exception::Exception);

      xdata::UnsignedInteger32 eventsInRU_;
      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger32 superFragmentSize_;
      xdata::UnsignedInteger64 eventCount_;
      xdata::UnsignedInteger32 lastEventNumber_;
      xdata::UnsignedInteger32 stopLocalInputAtEvent_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Unit,class Configuration,class StateMachine>
evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::ReadoutUnit
(
  xdaq::ApplicationStub* app,
  const std::string& appIcon
) :
  EvBApplication<Configuration,StateMachine>(app,appIcon)
{}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  eventRate_ = 0;
  superFragmentSize_ = 0;
  eventCount_ = 0;
  lastEventNumber_ = 0;
  stopLocalInputAtEvent_ = 0;

  appInfoSpaceParams.add("eventRate", &eventRate_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("superFragmentSize", &superFragmentSize_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("eventCount", &eventCount_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("lastEventNumber", &lastEventNumber_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("stopLocalInputAtEvent", &stopLocalInputAtEvent_, InfoSpaceItems::change);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  eventsInRU_ = 0;

  monitoringParams.add("eventsInRU", &eventsInRU_);

  input_->appendMonitoringItems(monitoringParams);
  buProxy_->appendMonitoringItems(monitoringParams);
  this->stateMachine_->appendMonitoringItems(monitoringParams);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_updateMonitoringInfo()
{
  input_->updateMonitoringItems();
  buProxy_->updateMonitoringItems();

  try
  {
    xdata::UnsignedInteger64 eventCount;
    eventCount.setValue( *this->monitoringInfoSpace_->find("eventCount") );

    xdata::UnsignedInteger64 fragmentCount;
    fragmentCount.setValue( *this->monitoringInfoSpace_->find("fragmentCount") );

    eventsInRU_ = eventCount>fragmentCount ? eventCount-fragmentCount : 0;
  }
  catch(xdata::exception::Exception)
  {
    eventsInRU_ = 0;
  }

  this->stateMachine_->updateMonitoringItems();
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "writeNextFragmentsToFile")
  {
    input_->writeNextFragmentsToFile(this->configuration_->writeNextFragmentsToFile);
  }
  else if (item == "maxTriggerRate")
  {
    input_->setMaxTriggerRate(this->configuration_->maxTriggerRate);
  }
  else if (item == "stopLocalInputAtEvent")
  {
    input_->stopLocalInputAtEvent(stopLocalInputAtEvent_);
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "eventRate")
  {
    try
    {
      eventRate_.setValue( *(this->monitoringInfoSpace_->find("eventRate")) );
    }
    catch(xdata::exception::Exception)
    {
      eventRate_ = 0;
    }
  }
  else if (item == "superFragmentSize")
  {
    try
    {
      superFragmentSize_.setValue( *(this->monitoringInfoSpace_->find("superFragmentSize")) );
    }
    catch(xdata::exception::Exception)
    {
      superFragmentSize_ = 0;
    }
  }
  else if (item == "eventCount")
  {
    try
    {
      eventCount_.setValue( *(this->monitoringInfoSpace_->find("eventCount")) );
    }
    catch(xdata::exception::Exception)
    {
      eventCount_ = 0;
    }
  }
  else if (item == "lastEventNumber")
  {
    try
    {
      lastEventNumber_.setValue( *(this->monitoringInfoSpace_->find("lastEventNumber")) );
    }
    catch(xdata::exception::Exception)
    {
      lastEventNumber_ = 0;
    }
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_bindI2oCallbacks()
{
  pt::frl::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::rawDataAvailable
  );

  i2o::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SHIP_FRAGMENTS_Callback,
    I2O_SHIP_FRAGMENTS,
    XDAQ_ORGANIZATION_ID
  );
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::rawDataAvailable
(
  toolbox::mem::Reference* bufRef,
  int originator,
  tcpla::MemoryCache* cache
)
throw (pt::frl::exception::Exception)
{
  try
  {
    input_->rawDataAvailable(bufRef,cache);
  }
  catch(xcept::Exception& e)
  {
    this->stateMachine_->processFSMEvent( Fail(e) );
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SHIP_FRAGMENTS_Callback
(
  toolbox::mem::Reference *bufRef
)
throw (i2o::exception::Exception)
{
  try
  {
    buProxy_->readoutMsgCallback(bufRef);
  }
  catch(xcept::Exception& e)
  {
    this->stateMachine_->processFSMEvent( Fail(e) );
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::bindNonDefaultXgiCallbacks()
{
  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::eventCountForLumiSection,
    "eventCountForLumiSection"
  );

  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::writeNextFragmentsToFile,
    "writeNextFragmentsToFile"
  );
}


template<class Unit,class Configuration,class StateMachine>
cgicc::table evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::getMainWebPage() const
{
  using namespace cgicc;

  table layoutTable;
  layoutTable.set("class","xdaq-evb-layout");
  layoutTable.add(colgroup()
                  .add(col())
                  .add(col().set("class","xdaq-evb-arrow"))
                  .add(col()));
  layoutTable.add(tr()
                  .add(td(this->getWebPageBanner()).set("colspan","3")));
  addComponentsToWebPage(layoutTable);

  return layoutTable;
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::writeNextFragmentsToFile
(
  xgi::Input  *in,
  xgi::Output *out
)
throw (xgi::exception::Exception)
{
  cgicc::Cgicc cgi(in);
  uint16_t count = 1;

  if ( xgi::Utils::hasFormElement(cgi,"count") )
    count = xgi::Utils::getFormElement(cgi, "count")->getIntegerValue();

  if ( xgi::Utils::hasFormElement(cgi,"fedid") )
  {
    const uint16_t fedId = xgi::Utils::getFormElement(cgi, "fedid")->getIntegerValue();
    input_->writeNextFragmentsToFile(count,fedId);
  }
  else
  {
    input_->writeNextFragmentsToFile(count);
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::eventCountForLumiSection
(
  xgi::Input  *in,
  xgi::Output *out
)
throw (xgi::exception::Exception)
{
  cgicc::Cgicc cgi(in);

  if ( xgi::Utils::hasFormElement(cgi,"ls") ) {
    const uint32_t ls = xgi::Utils::getFormElement(cgi, "ls")->getIntegerValue();
    *out << input_->getEventCountForLumiSection(ls);
  }
}

#endif // _evb_readoutunit_ReadoutUnit_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
