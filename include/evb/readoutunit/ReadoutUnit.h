#ifndef _evb_readoutunit_ReadoutUnit_h_
#define _evb_readoutunit_ReadoutUnit_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

#include "evb/EvBApplication.h"
#include "evb/InfoSpaceItems.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/StateMachine.h"
#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "interface/shared/i2ogevb2g.h"
#include "pt/utcp/frl/MemoryCache.h"
#include "pt/utcp/frl/Method.h"
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
    template<class Configuration,class Input>
    class ReadoutUnit : public EvBApplication< Configuration,StateMachine< ReadoutUnit<Configuration,Input> > >
    {
    public:
      
      ReadoutUnit
      (
        xdaq::ApplicationStub*,
        const std::string& appIcon
      );
      
      boost::shared_ptr<Input> getInput() const
      { return input_; }
      
      boost::shared_ptr< BUproxy<ReadoutUnit> > getBUproxy() const
      { return buProxy_; }
      
    private:
      
      virtual void do_bindI2oCallbacks();
      inline void dataReadyCallback(toolbox::mem::Reference*, int originator, pt::utcp::frl::MemoryCache*);
      inline void I2O_RU_SEND_Callback(toolbox::mem::Reference*);
      
      virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
      virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
      virtual void do_updateMonitoringInfo();
      
      virtual void do_handleItemChangedEvent(const std::string& item);
      virtual void do_handleItemRetrieveEvent(const std::string& item);
      
      virtual void bindNonDefaultXgiCallbacks();
      virtual void do_defaultWebPage(xgi::Output*);
      
      void requestFIFOWebPage(xgi::Input*, xgi::Output*);
      
      boost::shared_ptr<Input> input_;
      boost::shared_ptr< BUproxy<ReadoutUnit> > buProxy_;
      
      xdata::UnsignedInteger32 nbSuperFragmentsReady_;
      
    };
    
  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Configuration,class Input>
evb::readoutunit::ReadoutUnit<Configuration,Input>::ReadoutUnit
(
  xdaq::ApplicationStub* app,
  const std::string& appIcon
) :
EvBApplication< Configuration,StateMachine< ReadoutUnit<Configuration,Input> > >(app,appIcon)
{
  this->stateMachine_.reset( new StateMachine< ReadoutUnit<Configuration,Input> >(this) );
  input_.reset( new Input(app,this->configuration_) );
  buProxy_.reset( new BUproxy<ReadoutUnit>(this) );    
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  nbSuperFragmentsReady_ = 0;
  
  appInfoSpaceParams.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_, InfoSpaceItems::retrieve);
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  input_->appendMonitoringItems(monitoringParams);
  buProxy_->appendMonitoringItems(monitoringParams);
  this->stateMachine_->appendMonitoringItems(monitoringParams);
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_updateMonitoringInfo()
{
  input_->updateMonitoringItems();
  buProxy_->updateMonitoringItems();
  this->stateMachine_->updateMonitoringItems();
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "inputSource")
  {
    input_->inputSourceChanged();
  }
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_handleItemRetrieveEvent(const std::string& item)
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


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_bindI2oCallbacks()
{  
  // i2o::bind(
  //   this,
  //   &evb::dataReadyCallback,
  //   I2O_DATA_READY,
  //   XDAQ_ORGANIZATION_ID
  // );
  
  pt::utcp::frl::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Configuration,Input>::dataReadyCallback
  );
  
  i2o::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Configuration,Input>::I2O_RU_SEND_Callback,
    I2O_SHIP_FRAGMENTS,
    XDAQ_ORGANIZATION_ID
  );
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::dataReadyCallback
(
  toolbox::mem::Reference* bufRef,
  int originator,
  pt::utcp::frl::MemoryCache* cache
)
{
  input_->dataReadyCallback(bufRef,cache);
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::I2O_RU_SEND_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  buProxy_->rqstForFragmentsMsgCallback(bufRef);
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::bindNonDefaultXgiCallbacks()
{
  xgi::bind(
    this,
    &evb::readoutunit::ReadoutUnit<Configuration,Input>::requestFIFOWebPage,
    "requestFIFO"
  );
}


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::do_defaultWebPage
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


template<class Configuration,class Input>
void evb::readoutunit::ReadoutUnit<Configuration,Input>::requestFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  this->webPageHeader(out, "requestFIFO");

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
