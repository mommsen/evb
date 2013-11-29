#ifndef _evb_readoutunit_ReadoutUnit_h_
#define _evb_readoutunit_ReadoutUnit_h_

#include <boost/shared_ptr.hpp>

#include <string>

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

      void processI2O(const bool val)
      { processI2O_ = val; }

      typedef boost::shared_ptr< Input<Configuration> > InputPtr;
      InputPtr getInput() const
      { return input_; }

      typedef boost::shared_ptr< BUproxy<Unit> > BUproxyPtr;
      BUproxyPtr getBUproxy() const
      { return buProxy_; }

    protected:

      InputPtr input_;
      BUproxyPtr buProxy_;

    private:

      virtual void do_bindI2oCallbacks();
      inline void rawDataAvailable(toolbox::mem::Reference*, int originator, tcpla::MemoryCache*);
      inline void I2O_SUPER_FRAGMENT_READY_Callback(toolbox::mem::Reference*);
      inline void I2O_SHIP_FRAGMENTS_Callback(toolbox::mem::Reference*);

      virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
      virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
      virtual void do_updateMonitoringInfo();

      virtual void do_handleItemChangedEvent(const std::string& item);
      virtual void do_handleItemRetrieveEvent(const std::string& item);

      virtual void bindNonDefaultXgiCallbacks();
      virtual void do_defaultWebPage(xgi::Output*) const;

      void fragmentRequestFIFOWebPage(xgi::Input*, xgi::Output*);
      void superFragmentFIFOWebPage(xgi::Input*, xgi::Output*);
      void eventCountForLumiSection(xgi::Input*, xgi::Output*);

      bool processI2O_;

      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger32 superFragmentSize_;

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

  appInfoSpaceParams.add("eventRate", &eventRate_, InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("superFragmentSize", &superFragmentSize_, InfoSpaceItems::retrieve);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  input_->appendMonitoringItems(monitoringParams);
  buProxy_->appendMonitoringItems(monitoringParams);
  this->stateMachine_->appendMonitoringItems(monitoringParams);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_updateMonitoringInfo()
{
  input_->updateMonitoringItems();
  buProxy_->updateMonitoringItems();
  this->stateMachine_->updateMonitoringItems();
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "inputSource")
  {
    input_->inputSourceChanged();
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
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SUPER_FRAGMENT_READY_Callback,
    I2O_SUPER_FRAGMENT_READY,
    XDAQ_ORGANIZATION_ID
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
{
  if ( processI2O_ )
    input_->rawDataAvailable(bufRef,cache);
  else
  {
    bufRef->release();
    LOG4CPLUS_WARN(this->logger_, "Received a rawDataAvailable callback while not running.");
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SUPER_FRAGMENT_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  if ( processI2O_ )
    input_->superFragmentReady(bufRef);
  else
  {
    bufRef->release();
    LOG4CPLUS_WARN(this->logger_, "Received an I2O_SUPER_FRAGMENT_READY message while not running.");
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SHIP_FRAGMENTS_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  if ( processI2O_ )
    buProxy_->readoutMsgCallback(bufRef);
  else
  {
    bufRef->release();
    LOG4CPLUS_WARN(this->logger_, "Received an I2O_SHIP_FRAGMENTS message while not running.");
  }
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::bindNonDefaultXgiCallbacks()
{
  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::fragmentRequestFIFOWebPage,
    "fragmentRequestFIFO"
  );

  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::superFragmentFIFOWebPage,
    "superFragmentFIFO"
  );

  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::eventCountForLumiSection,
    "eventCountForLumiSection"
  );
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_defaultWebPage
(
  xgi::Output *out
) const
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  input_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  buProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::fragmentRequestFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  this->webPageHeader(out, "fragmentRequestFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  this->webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  buProxy_->printFragmentRequestFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::superFragmentFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  this->webPageHeader(out, "fragmentRequestFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  this->webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  input_->printSuperFragmentFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  *out << "</table>"                                            << std::endl;

  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::eventCountForLumiSection
(
  xgi::Input  *in,
  xgi::Output *out
)
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
