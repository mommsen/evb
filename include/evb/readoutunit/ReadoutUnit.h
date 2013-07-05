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
#include "xgi/Output.h"


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
      
      boost::shared_ptr< Input<Configuration> > getInput() const
      { return input_; }
      
      boost::shared_ptr< BUproxy<Unit> > getBUproxy() const
      { return buProxy_; }
      
    protected:
      
      boost::shared_ptr< Input<Configuration> > input_;
      boost::shared_ptr< BUproxy<Unit> > buProxy_;
      
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
      virtual void do_defaultWebPage(xgi::Output*);
      
      void fragmentRequestFIFOWebPage(xgi::Input*, xgi::Output*);
      
      xdata::UnsignedInteger32 nbSuperFragmentsReady_;
      
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
  nbSuperFragmentsReady_ = 0;
  
  appInfoSpaceParams.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_, InfoSpaceItems::retrieve);
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
  if (item == "nbSuperFragmentsReady")
  {
    try
    {
      nbSuperFragmentsReady_.setValue( *(this->monitoringInfoSpace_->find("nbSuperFragmentsReady")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbSuperFragmentsReady_ = 0;
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
  input_->rawDataAvailable(bufRef,cache);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SUPER_FRAGMENT_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  input_->superFragmentReady(bufRef);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::I2O_SHIP_FRAGMENTS_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  buProxy_->rqstForFragmentsMsgCallback(bufRef);
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::bindNonDefaultXgiCallbacks()
{
  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::fragmentRequestFIFOWebPage,
    "fragmentRequestFIFO"
  );
}


template<class Unit,class Configuration,class StateMachine>
void evb::readoutunit::ReadoutUnit<Unit,Configuration,StateMachine>::do_defaultWebPage
(
  xgi::Output *out
)
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


#endif // _evb_readoutunit_ReadoutUnit_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
