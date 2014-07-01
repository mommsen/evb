#ifndef _evb_EvBApplication_h_
#define _evb_EvBApplication_h_

#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>
#include <time.h>

#include "cgicc/HTMLClasses.h"
#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"
#include "evb/EvBStateMachine.h"
#include "evb/version.h"
#include "i2o/Method.h"
#include "log4cplus/loggingmacros.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/net/URN.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdaq/ApplicationStub.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/Application.h"
#include "xdaq2rc/SOAPParameterExtractor.hh"
#include "xdata/ActionListener.h"
#include "xdata/InfoSpace.h"
#include "xdata/InfoSpaceFactory.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/framework/Method.h"
#include "xgi/framework/UIManager.h"
#include "xgi/Input.h"
#include "xgi/Output.h"
#include "xoap/MessageFactory.h"
#include "xoap/MessageReference.h"
#include "xoap/Method.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPEnvelope.h"
#include "xoap/SOAPName.h"
#include "xoap/SOAPPart.h"


namespace evb {

  /**
   * \ingroup xdaqApps
   * \brief Generic evb application.
   */
  template<class Configuration,class StateMachine>
  class EvBApplication :
    public xdaq::Application, public xgi::framework::UIManager, public xdata::ActionListener
  {
  public:

    /**
     * Constructor.
     */
    EvBApplication
    (
      xdaq::ApplicationStub*,
      const std::string& appIcon
    );

    toolbox::net::URN getURN() const { return urn_; }
    std::string getIdentifier(const std::string& suffix = "") const;
    boost::shared_ptr<Configuration> getConfiguration() const { return configuration_; }
    boost::shared_ptr<StateMachine> getStateMachine() const { return stateMachine_; }

    typedef boost::function<cgicc::div()> QueueContentFunction;
    void registerQueueCallback(const std::string name, QueueContentFunction);

  protected:

    void initialize();

    virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&) = 0;
    virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&) = 0;
    virtual void do_updateMonitoringInfo() = 0;

    virtual void do_handleItemChangedEvent(const std::string& item) {};
    virtual void do_handleItemRetrieveEvent(const std::string& item) {};

    virtual void do_bindI2oCallbacks() {};

    virtual void bindNonDefaultXgiCallbacks() {};
    virtual cgicc::table getMainWebPage() const = 0;
    cgicc::div getWebPageHeader() const;
    cgicc::div getWebPageBanner() const;

    toolbox::mem::Pool* getFastControlMsgPool() const;
    xoap::MessageReference createFsmSoapResponseMsg
    (
      const std::string& event,
      const std::string& state
    ) const;

    xdata::InfoSpace *monitoringInfoSpace_;

    const boost::shared_ptr<Configuration> configuration_;
    boost::shared_ptr<StateMachine> stateMachine_;
    xdaq2rc::SOAPParameterExtractor soapParameterExtractor_;

    const toolbox::net::URN urn_;
    const std::string xmlClass_;
    xdata::UnsignedInteger32 instance_;
    xdata::String stateName_;
    xdata::UnsignedInteger32 monitoringSleepSec_;


  private:

    void initApplicationInfoSpace();
    void initMonitoringInfoSpace();
    void startMonitoring();
    void bindI2oCallbacks();
    void bindSoapCallbacks();
    void bindXgiCallbacks();
    xoap::MessageReference processSoapFsmEvent(xoap::MessageReference msg) throw (xoap::exception::Exception);

    void startMonitoringWorkloop();
    bool updateMonitoringInfo(toolbox::task::WorkLoop*);

    void appendApplicationInfoSpaceItems(InfoSpaceItems&);
    void appendMonitoringInfoSpaceItems(InfoSpaceItems&);
    void actionPerformed(xdata::Event&);
    void handleItemChangedEvent(const std::string& item);
    void handleItemRetrieveEvent(const std::string& item);

    void defaultWebPage(xgi::Input*, xgi::Output*) throw (xgi::exception::Exception);
    void queueWebPage(xgi::Input*, xgi::Output*) throw (xgi::exception::Exception);
    std::string getCurrentTimeUTC() const;

    typedef std::map<std::string,QueueContentFunction> QueueContents;
    QueueContents queueContents_;

  }; // template class EvBApplication

} // namespace evb


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Configuration,class StateMachine>
evb::EvBApplication<Configuration,StateMachine>::EvBApplication
(
  xdaq::ApplicationStub* app,
  const std::string& appIcon
) :
  xdaq::Application(app),
  xgi::framework::UIManager(this),
  configuration_(new Configuration()),
  soapParameterExtractor_(this),
  urn_(getApplicationDescriptor()->getURN()),
  xmlClass_(getApplicationDescriptor()->getClassName()),
  instance_(getApplicationDescriptor()->getInstance())
{
  getApplicationDescriptor()->setAttribute("icon", appIcon);
  getApplicationDescriptor()->setAttribute("icon16x16", appIcon);
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::initialize()
{
  stateMachine_->initiate();

  initApplicationInfoSpace();
  initMonitoringInfoSpace();
  startMonitoring();

  bindSoapCallbacks();
  bindI2oCallbacks();
  bindXgiCallbacks();
}


template<class Configuration,class StateMachine>
std::string evb::EvBApplication<Configuration,StateMachine>::getIdentifier(const std::string& suffix) const
{
  std::ostringstream identifier;
  identifier << xmlClass_ << "_" << instance_.value_;
  if ( !suffix.empty() )
    identifier << "/" << suffix;

  return identifier.str();
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::initApplicationInfoSpace()
{
  try
  {
    InfoSpaceItems appInfoSpaceParams;
    xdata::InfoSpace *appInfoSpace = getApplicationInfoSpace();

    appendApplicationInfoSpaceItems(appInfoSpaceParams);

    appInfoSpaceParams.putIntoInfoSpace(appInfoSpace, this);
  }
  catch(xcept::Exception& e)
  {
    const std::string msg = "Failed to put parameters into application info space";
    LOG4CPLUS_ERROR(getApplicationLogger(),
                    msg << xcept::stdformat_exception_history(e));

    // Notify the sentinel
    XCEPT_DECLARE_NESTED(exception::Monitoring,
                         sentinelException, msg, e);
    notifyQualified("error",sentinelException);
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::appendApplicationInfoSpaceItems
(
  InfoSpaceItems& params
)
{
  stateName_ = "Halted";
  monitoringSleepSec_ = 1;

  params.add("stateName", &stateName_, InfoSpaceItems::retrieve);
  params.add("monitoringSleepSec", &monitoringSleepSec_);

  stateMachine_->appendConfigurationItems(params);

  do_appendApplicationInfoSpaceItems(params);

  configuration_->addToInfoSpace( params, getApplicationDescriptor()->getInstance(), getApplicationContext() );
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::initMonitoringInfoSpace()
{
  try
  {
    InfoSpaceItems monitoringParams;

    appendMonitoringInfoSpaceItems(monitoringParams);

    // Create info space for monitoring
    toolbox::net::URN urn = createQualifiedInfoSpace(xmlClass_);
    monitoringInfoSpace_ = xdata::getInfoSpaceFactory()->get(urn.toString());

    monitoringParams.putIntoInfoSpace(monitoringInfoSpace_, this);
  }
  catch(xcept::Exception& e)
  {
    const std::string msg = "Failed to put parameters into monitoring info space";
    LOG4CPLUS_ERROR(getApplicationLogger(),
                    msg << xcept::stdformat_exception_history(e));

    // Notify the sentinel
    XCEPT_DECLARE_NESTED(exception::Monitoring,
                         sentinelException, msg, e);
    notifyQualified("error",sentinelException);
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& items
)
{
  do_appendMonitoringInfoSpaceItems(items);
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::actionPerformed(xdata::Event& ispaceEvent)
{
  std::ostringstream msg;
  msg << "Failed to perform action for " << ispaceEvent.type();

  try
  {
    if (ispaceEvent.type() == "ItemChangedEvent")
    {
      const std::string item =
        dynamic_cast<xdata::ItemChangedEvent&>(ispaceEvent).itemName();
      handleItemChangedEvent(item);
    }
    else if (ispaceEvent.type() == "ItemRetrieveEvent")
    {
      const std::string item =
        dynamic_cast<xdata::ItemRetrieveEvent&>(ispaceEvent).itemName();
      handleItemRetrieveEvent(item);
    }
  }
  catch(xcept::Exception& e)
  {
    XCEPT_DECLARE_NESTED(exception::Monitoring,
                         sentinelException, msg.str(), e);
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
  {
    msg << ": " << e.what();
    XCEPT_DECLARE(exception::Monitoring,
                  sentinelException, msg.str());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg << ": unknown exception";
    XCEPT_DECLARE(exception::Monitoring,
                  sentinelException, msg.str());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::handleItemChangedEvent(const std::string& item)
{
  do_handleItemChangedEvent(item);
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::handleItemRetrieveEvent(const std::string& item)
{
  if (item == "stateName")
  {
    stateName_ = stateMachine_->getStateName();
  }
  else
  {
    do_handleItemRetrieveEvent(item);
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::bindI2oCallbacks()
{
  do_bindI2oCallbacks();
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::bindSoapCallbacks()
{
  typename StateMachine::SoapFsmEvents soapFsmEvents =
    stateMachine_->getSoapFsmEvents();

  for (typename StateMachine::SoapFsmEvents::const_iterator it = soapFsmEvents.begin(),
         itEnd = soapFsmEvents.end(); it != itEnd; ++it)
  {
    xoap::bind(
      this,
      &evb::EvBApplication<Configuration,StateMachine>::processSoapFsmEvent,
      *it,
      XDAQ_NS_URI
    );
  }
}


template<class Configuration,class StateMachine>
xoap::MessageReference evb::EvBApplication<Configuration,StateMachine>::processSoapFsmEvent
(
  xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
  std::string event = "";
  std::string newState = "unknown";

  std::ostringstream errorMsg;
  errorMsg << "Failed to extract FSM event and parameters from SOAP message: ";

  try
  {
    event = soapParameterExtractor_.extractParameters(msg);
  }
  catch(xcept::Exception& e)
  {
    msg->writeTo(errorMsg);
    XCEPT_DECLARE_NESTED(exception::SOAP, sentinelException, errorMsg.str(), e);
    newState = stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
  {
    errorMsg << e.what() << ": ";
    msg->writeTo(errorMsg);
    XCEPT_DECLARE(exception::SOAP, sentinelException, errorMsg.str());
    newState = stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg->writeTo(errorMsg);
    XCEPT_DECLARE(exception::SOAP, sentinelException, errorMsg.str());
    newState = stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  stateMachine_->processSoapEvent(event, newState);

  return createFsmSoapResponseMsg(event, newState);
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::startMonitoring()
{
  std::ostringstream msg;
  msg << "Failed to start monitoring work loop";

  try
  {
    startMonitoringWorkloop();
  }
  catch(xcept::Exception& e)
  {
    XCEPT_DECLARE_NESTED(exception::WorkLoop,
                         sentinelException, msg.str(), e);
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(std::exception& e)
  {
    msg << ": " << e.what();
    XCEPT_DECLARE(exception::WorkLoop,
                  sentinelException, msg.str());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    msg << ": unknown exception";
    XCEPT_DECLARE(exception::WorkLoop,
                  sentinelException, msg.str());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::startMonitoringWorkloop()
{
  const std::string monitoringWorkLoopName( getIdentifier("monitoring") );

  toolbox::task::WorkLoop* monitoringWorkLoop = toolbox::task::getWorkLoopFactory()->getWorkLoop
    (
      monitoringWorkLoopName,
      "waiting"
    );

  const std::string monitoringActionName( getIdentifier("monitoringAction") );

  toolbox::task::ActionSignature* monitoringActionSignature =
    toolbox::task::bind
    (
      this,
      &evb::EvBApplication<Configuration,StateMachine>::updateMonitoringInfo,
      monitoringActionName
    );

  try
  {
    monitoringWorkLoop->submit(monitoringActionSignature);
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::WorkLoop,
                  "Failed to submit action to work loop: " + monitoringWorkLoopName,
                  e);
  }

  try
  {
    monitoringWorkLoop->activate();
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::WorkLoop,
                  "Failed to activate work loop: " + monitoringWorkLoopName, e);
  }
}


template<class Configuration,class StateMachine>
bool evb::EvBApplication<Configuration,StateMachine>::updateMonitoringInfo
(
  toolbox::task::WorkLoop* wl
)
{
  std::string errorMsg = "Failed to update monitoring counters: ";

  try
  {
    monitoringInfoSpace_->lock();

    do_updateMonitoringInfo();

    monitoringInfoSpace_->unlock();
  }
  catch(xcept::Exception& e)
  {
    monitoringInfoSpace_->unlock();

    LOG4CPLUS_ERROR(getApplicationLogger(),
                    errorMsg << xcept::stdformat_exception_history(e));

    XCEPT_DECLARE_NESTED(exception::Monitoring,
                         sentinelException, errorMsg, e);
    notifyQualified("error",sentinelException);
  }
  catch(std::exception &e)
  {
    monitoringInfoSpace_->unlock();

    errorMsg += e.what();

    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);

    XCEPT_DECLARE(exception::Monitoring,
                  sentinelException, errorMsg );
    notifyQualified("error",sentinelException);
  }
  catch(...)
  {
    monitoringInfoSpace_->unlock();

    errorMsg += "Unknown exception";

    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);

    XCEPT_DECLARE(exception::Monitoring,
                  sentinelException, errorMsg );
    notifyQualified("error",sentinelException);
  }

  ::sleep(monitoringSleepSec_.value_);

  // Reschedule this action code
  return true;
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::bindXgiCallbacks()
{
  xgi::framework::deferredbind(this, this,
                               &evb::EvBApplication<Configuration,StateMachine>::defaultWebPage,
                               "Default");

  bindNonDefaultXgiCallbacks();
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::defaultWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
throw (xgi::exception::Exception)
{
  *out << getWebPageHeader();
  *out << getMainWebPage();
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::registerQueueCallback
(
  const std::string name,
  QueueContentFunction queueContentFunction
)
{
  const QueueContents::iterator pos = queueContents_.find(name);
  if ( pos == queueContents_.end() )
  {
    xgi::framework::deferredbind(this, this,
                                 &evb::EvBApplication<Configuration,StateMachine>::queueWebPage,
                                 name);

    queueContents_.insert( QueueContents::value_type(name,queueContentFunction) );
  }
  else
  {
    pos->second = queueContentFunction;
  }
}


template<class Configuration,class StateMachine>
void evb::EvBApplication<Configuration,StateMachine>::queueWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
throw (xgi::exception::Exception)
{
  using namespace cgicc;

  const std::string name = in->getenv("PATH_INFO");
  const QueueContents::const_iterator pos = queueContents_.find(name);

  cgicc::div queueDetail;
  if ( pos == queueContents_.end() )
    queueDetail.add(p("No callback found for queue "+name));
  else
    queueDetail = pos->second();

  table layoutTable;
  layoutTable.set("class","xdaq-evb-layout");
  layoutTable.add(tr()
                  .add(td(getWebPageBanner())));
  layoutTable.add(tr()
                  .add(td(queueDetail)));

  *out << getWebPageHeader();
  *out << layoutTable;
}


template<class Configuration,class StateMachine>
cgicc::div evb::EvBApplication<Configuration,StateMachine>::getWebPageHeader() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(script("$(\"head\").append(\"<link href=\\\"/evb/html/xdaq-evb.css\\\" rel=\\\"stylesheet\\\"/>\");"));
  return div;
}


template<class Configuration,class StateMachine>
cgicc::div evb::EvBApplication<Configuration,StateMachine>::getWebPageBanner() const
{
  using namespace cgicc;

  table infoSpaceTable;
  infoSpaceTable.set("align","right");
  infoSpaceTable.add(tr()
                     .add(td("InfoSpaces: "))
                     .add(td()
                          .add(a("Application")
                               .set("href","/urn:xdaq-application:service=hyperdaq/infospaceQuery?name="+urn_.toString())))
                     .add(td()
                          .add(a("Monitoring")
                               .set("href","/urn:xdaq-application:service=hyperdaq/infospaceQuery?name="+monitoringInfoSpace_->name()))));

  table banner;
  banner.set("class","xdaq-evb-banner");

  banner.add(tr()
             .add(td(getIdentifier()))
             .add(td(infoSpaceTable)));

  banner.add(tr()
             .add(td("run "+boost::lexical_cast<std::string>(stateMachine_->getRunNumber())))
             .add(td("Version "+evb::version)));

  const std::string stateName = stateMachine_->getStateName();
  banner.add(tr()
             .add(td(stateName))
             .add(td("Page last updated: "+getCurrentTimeUTC()+" UTC")));

  if ( stateName == "Failed" )
  {
    banner.add(tr().set("class","xdaq-error")
               .add(td(stateMachine_->getReasonForFailed()).set("colspan","2")));
  }

  cgicc::div div;
  div.set("class","xdaq-header");
  div.add(banner);
  return div;
}


template<class Configuration,class StateMachine>
xoap::MessageReference evb::EvBApplication<Configuration,StateMachine>::createFsmSoapResponseMsg
(
  const std::string& event,
  const std::string& state
) const
{
  try
  {
    xoap::MessageReference message = xoap::createMessage();
    xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
    xoap::SOAPBody body = envelope.getBody();
    std::string responseString = event + "Response";
    xoap::SOAPName responseName =
      envelope.createName(responseString, "xdaq", XDAQ_NS_URI);
    xoap::SOAPBodyElement responseElement =
      body.addBodyElement(responseName);
    xoap::SOAPName stateName =
      envelope.createName("state", "xdaq", XDAQ_NS_URI);
    xoap::SOAPElement stateElement =
      responseElement.addChildElement(stateName);
    xoap::SOAPName attributeName =
      envelope.createName("stateName", "xdaq", XDAQ_NS_URI);

    stateElement.addAttribute(attributeName, state);

    return message;
  }
  catch(xcept::Exception& e)
  {
    XCEPT_RETHROW(exception::SOAP,
                  "Failed to create FSM SOAP response message for event:" +
                  event + " and result state:" + state,  e);
  }
}


template<class Configuration,class StateMachine>
toolbox::mem::Pool* evb::EvBApplication<Configuration,StateMachine>::getFastControlMsgPool() const
{
  toolbox::mem::Pool* fastCtrlMsgPool = 0;

  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", configuration_->sendPoolName);
    fastCtrlMsgPool = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch(toolbox::mem::exception::MemoryPoolNotFound)
  {
    const std::string fastCtrlMsgPoolName( getIdentifier(" fast control message pool") );

    toolbox::net::URN urn("toolbox-mem-pool", fastCtrlMsgPoolName);
    toolbox::mem::HeapAllocator* a = new toolbox::mem::HeapAllocator();

    fastCtrlMsgPool = toolbox::mem::getMemoryPoolFactory()->createPool(urn, a);
  }
  catch(toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory, "Failed to create fast control message memory pool", e);
  }

  return fastCtrlMsgPool;
}


template<class Configuration,class StateMachine>
std::string evb::EvBApplication<Configuration,StateMachine>::getCurrentTimeUTC() const
{
  time_t now;
  struct tm tm;
  char buf[30];

  if (time(&now) != ((time_t)-1))
  {
    gmtime_r(&now,&tm);
    asctime_r(&tm,buf);
  }
  return buf;
}


#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
