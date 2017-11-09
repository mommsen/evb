#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/Constants.h"
#include "evb/DummyFEROL.h"
#include "evb/dummyFEROL/States.h"
#include "evb/dummyFEROL/StateMachine.h"
#include "evb/Exception.h"
#include "evb/version.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

#include <boost/lexical_cast.hpp>


evb::test::DummyFEROL::DummyFEROL(xdaq::ApplicationStub* app) :
  EvBApplication<dummyFEROL::Configuration,dummyFEROL::StateMachine>(app,"/evb/images/ferol64x64.gif"),
  sockfd_(0),
  doProcessing_(false),
  active_(false),
  lastResync_(0)
{
  stateMachine_.reset( new dummyFEROL::StateMachine(this) );

  resetMonitoringCounters();
  startWorkLoops();

  initialize();

  LOG4CPLUS_INFO(getApplicationLogger(), "End of constructor");
}


evb::test::DummyFEROL::~DummyFEROL()
{
  closeConnection();

  if ( workLoop_ && workLoop_->isActive() )
    workLoop_->cancel();
}


void evb::test::DummyFEROL::do_appendApplicationInfoSpaceItems
(
  InfoSpaceItems& appInfoSpaceParams
)
{
  stopAtEvent_ = 0;
  skipNbEvents_ = 0;
  resyncAtEvent_ = 1<<25;
  duplicateNbEvents_ = 0;
  corruptNbEvents_ = 0;
  nbCRCerrors_ = 0;
  nbBXerrors_ = 0;

  appInfoSpaceParams.add("lastEventNumber", &lastEventNumber_);
  appInfoSpaceParams.add("fragmentRate", &fragmentRate_);
  appInfoSpaceParams.add("stopAtEvent", &stopAtEvent_);
  appInfoSpaceParams.add("resyncAtEvent", &resyncAtEvent_);
  appInfoSpaceParams.add("skipNbEvents", &skipNbEvents_);
  appInfoSpaceParams.add("duplicateNbEvents", &duplicateNbEvents_);
  appInfoSpaceParams.add("corruptNbEvents", &corruptNbEvents_);
  appInfoSpaceParams.add("nbCRCerrors", &nbCRCerrors_);
  appInfoSpaceParams.add("nbBXerrors", &nbBXerrors_);
}


void evb::test::DummyFEROL::do_appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& monitoringParams
)
{
  lastEventNumber_ = 0;
  throughput_ = 0;
  frameRate_ = 0;
  fragmentRate_ = 0;
  fragmentSize_ = 0;
  fragmentSizeStdDev_ = 0;

  monitoringParams.add("lastEventNumber", &lastEventNumber_);
  monitoringParams.add("throughput", &throughput_);
  monitoringParams.add("frameRate", &frameRate_);
  monitoringParams.add("fragmentRate", &fragmentRate_);
  monitoringParams.add("fragmentSize", &fragmentSize_);
  monitoringParams.add("fragmentSizeStdDev", &fragmentSizeStdDev_);
}


void evb::test::DummyFEROL::do_updateMonitoringInfo()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

  const double deltaT = dataMonitoring_.deltaT();
  throughput_ = dataMonitoring_.throughput(deltaT);
  frameRate_ = dataMonitoring_.i2oRate(deltaT);
  fragmentRate_ = dataMonitoring_.logicalRate(deltaT);
  fragmentSize_ = dataMonitoring_.size();
  fragmentSizeStdDev_ = dataMonitoring_.sizeStdDev();
  dataMonitoring_.reset();
}


void evb::test::DummyFEROL::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "maxTriggerRate")
  {
    fragmentGenerator_.setMaxTriggerRate(this->configuration_->maxTriggerRate);
  }
}


cgicc::table evb::test::DummyFEROL::getMainWebPage() const
{
  using namespace cgicc;

  table layoutTable;
  layoutTable.set("class","xdaq-evb-layout");
  layoutTable.add(colgroup()
                  .add(col())
                  .add(col().set("class","xdaq-evb-arrow"))
                  .add(col()));
  layoutTable.add(tr()
                  .add(td(this->getWebPageBanner()))
                  .add(td(" "))
                  .add(td(" ")));
  layoutTable.add(tr()
                  .add(td(getHtmlSnipped()).set("class","xdaq-evb-component"))
                  .add(td(" "))
                  .add(td(" ")));

  return layoutTable;
}


cgicc::div evb::test::DummyFEROL::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  div.add(p("FEROL"));

  {
    table table;

    boost::mutex::scoped_lock sl(dataMonitoringMutex_);

    table.add(tr()
              .add(td("last event number"))
              .add(td(boost::lexical_cast<std::string>(lastEventNumber_.value_))));
    table.add(tr()
              .add(td("last resync"))
              .add(td(boost::lexical_cast<std::string>(lastResync_))));
    table.add(tr()
              .add(td("throughput (MB/s)"))
              .add(td(doubleToString(throughput_.value_ / 1e6,2))));
    table.add(tr()
              .add(td("rate (fragments/s)"))
              .add(td(boost::lexical_cast<std::string>(fragmentRate_))));
    table.add(tr()
              .add(td("rate (frame/s)"))
              .add(td(doubleToString(frameRate_,2))));
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << fragmentSize_.value_ / 1e3 << " +/- " << fragmentSizeStdDev_.value_ / 1e3;
      table.add(tr()
                .add(td("frame size (kB)"))
                .add(td(str.str())));
    }
    div.add(table);
  }

  return div;
}


void evb::test::DummyFEROL::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  dataMonitoring_.reset();
  lastEventNumber_ = 0;
  lastResync_ = 0;
}


void evb::test::DummyFEROL::configure()
{
  fragmentGenerator_.configure(
    configuration_->fedId,
    configuration_->usePlayback,
    configuration_->playbackDataFile,
    configuration_->frameSize,
    configuration_->fedSize,
    configuration_->computeCRC,
    configuration_->fedSizeStdDev,
    configuration_->minFedSize,
    configuration_->maxFedSize,
    configuration_->frameSize*configuration_->fragmentFIFOCapacity,
    configuration_->fakeLumiSectionDuration,
    configuration_->maxTriggerRate
  );

  openConnection();
}


void evb::test::DummyFEROL::openConnection()
{
  sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if ( sockfd_ < 0 )
  {
    XCEPT_RAISE(exception::TCP, "Failed to open socket");
  }

  // Allow socket to reuse the address
  int yes = 1;
  if ( setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to set SO_REUSEADDR on socket: " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  // Allow socket to reuse the port
  if ( setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to set SO_REUSEPORT on socket: " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  // Set connection Abort on close
  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;
  if ( setsockopt(sockfd_, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to set SO_LINGER on socket: " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  sockaddr_in sa_local;
  memset(&sa_local, 0, sizeof(sa_local));
  sa_local.sin_family = AF_INET;
  sa_local.sin_port = htons(configuration_->sourcePort.value_);
  sa_local.sin_addr.s_addr = inet_addr(configuration_->sourceHost.value_.c_str());

  if ( bind(sockfd_, (struct sockaddr*)&sa_local, sizeof(struct sockaddr)) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to bind socket to local port " << configuration_->sourceHost.value_ << ":" << configuration_->sourcePort;
    msg << " : " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  addrinfo hints, *servinfo;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  char str[8];
  sprintf(str,"%d",configuration_->destinationPort.value_);
  const int result = getaddrinfo(
    configuration_->destinationHost.value_.c_str(),
    (char*)&str,
    &hints,&servinfo);
  if ( result != 0 )
  {
    std::ostringstream msg;
    msg << "Failed to get server info for " << configuration_->destinationHost.value_ << ":" << configuration_->destinationPort;
    msg << " : " << gai_strerror(result);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  if ( connect(sockfd_,servinfo->ai_addr,servinfo->ai_addrlen) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to connect to " << configuration_->destinationHost.value_ << ":" << configuration_->destinationPort;
    msg << " : " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  // Set socket to non-blocking
  const int flags = fcntl(sockfd_, F_GETFL, 0);
  if ( fcntl(sockfd_, F_SETFL, flags|O_NONBLOCK) < 0 )
  {
    closeConnection();
    std::ostringstream msg;
    msg << "Failed to set O_NONBLOCK on socket: " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }
}


void evb::test::DummyFEROL::startProcessing()
{
  resetMonitoringCounters();
  stopAtEvent_ = 0;
  doProcessing_ = true;
  fragmentGenerator_.reset();
  workLoop_->submit(action_);
}


void evb::test::DummyFEROL::drain()
{
  if ( stopAtEvent_.value_ == 0 ) stopAtEvent_.value_ = lastEventNumber_;
  while ( active_ ) ::usleep(1000);
}


void evb::test::DummyFEROL::stopProcessing()
{
  doProcessing_ = false;
  while ( active_ ) ::usleep(1000);
}


void evb::test::DummyFEROL::closeConnection()
{
  if ( close(sockfd_) < 0 )
  {
    std::ostringstream msg;
    msg << "Failed to close socket to " << configuration_->destinationHost.value_ << ":" << configuration_->destinationPort;
    msg << " : " << strerror(errno);
    XCEPT_RAISE(exception::TCP, msg.str());
  }
  sockfd_ = 0;
}


void evb::test::DummyFEROL::startWorkLoops()
{
  try
  {
    workLoop_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( getIdentifier("generating"), "waiting" );

    action_ =
      toolbox::task::bind(this, &evb::test::DummyFEROL::generating,
                          getIdentifier("generatingAction") );

    if ( ! workLoop_->isActive() )
      workLoop_->activate();
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Generating'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::test::DummyFEROL::generating(toolbox::task::WorkLoop *wl)
{
  if ( ! doProcessing_ ) return false;

  active_ = true;

  try
  {
    while ( doProcessing_ &&
            (stopAtEvent_.value_ == 0 || lastEventNumber_.value_ < stopAtEvent_.value_) )
    {
      if ( resyncAtEvent_.value_ < 1<<25 )
      {
        fragmentGenerator_.resyncAtEvent(resyncAtEvent_);
        lastResync_ = resyncAtEvent_;
        resyncAtEvent_ = 1<<25;
      }

      toolbox::mem::Reference* bufRef = 0;

      if ( fragmentGenerator_.getData(bufRef,stopAtEvent_.value_,lastEventNumber_.value_,
                                      skipNbEvents_.value_,duplicateNbEvents_.value_,
                                      corruptNbEvents_.value_,nbCRCerrors_.value_,nbBXerrors_.value_) )
      {
        updateCounters(bufRef);
        sendData(bufRef);
        ::usleep(1000);
      }
    }
  }
  catch(xcept::Exception &e)
  {
    active_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    active_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    active_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  active_ = false;

  return doProcessing_;
}


inline void evb::test::DummyFEROL::updateCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

  const uint32_t payload = bufRef->getDataSize();

  ++dataMonitoring_.i2oCount;
  dataMonitoring_.logicalCount += configuration_->frameSize/(configuration_->fedSize+sizeof(ferolh_t));
  dataMonitoring_.sumOfSizes += payload;
  dataMonitoring_.sumOfSquares += payload*payload;
}


inline void evb::test::DummyFEROL::sendData(toolbox::mem::Reference* bufRef)
{
  char* buf = (char*)bufRef->getDataLocation();
  ssize_t len = bufRef->getDataSize();

  while ( len > 0 && doProcessing_ )
  {
    const ssize_t written = write(sockfd_,buf,len);
    if ( written < 0 )
    {
      if ( errno == EWOULDBLOCK )
      {
        ::usleep(100);
      }
      else
      {
        std::ostringstream msg;
        msg << "Failed to send data to " << configuration_->destinationHost.value_ << ":" << configuration_->destinationPort;
        msg << " : " << strerror(errno);
        XCEPT_RAISE(exception::TCP, msg.str());
      }
    }
    else
    {
      len -= written;
      buf += written;
    }
  }
  bufRef->release();
}


/**
 * Provides the factory method for the instantiation of DummyFEROL applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::test::DummyFEROL)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
