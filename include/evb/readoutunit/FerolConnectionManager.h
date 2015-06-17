#ifndef _evb_readoutunit_FerolConnectionManager_h_
#define _evb_readoutunit_FerolConnectionManager_h_

#include <netdb.h>
#include <set>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <vector>

#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/SocketStream.h"
#include "pt/blit/InputPipe.h"
#include "pt/blit/PipeAdvertisement.h"
#include "pt/blit/PipeService.h"
#include "pt/blit/PipeServiceListener.h"
#include "pt/PeerTransportAgent.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xcept/tools.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Manage FEROL connections using pt::blit
    */

    template<class ReadoutUnit, class Configuration>
    class FerolConnectionManager : public pt::blit::PipeServiceListener, public pt::blit::InputPipeListener, public toolbox::lang::Class
    {
    public:

      FerolConnectionManager(ReadoutUnit*);

      ~FerolConnectionManager();

      /**
       * Callback from PipeServiceListener for advertisement of PSP index
       */
      void pipeServiceEvent(pt::blit::PipeAdvertisement&);

      /**
       * Callback from InputPipeListener for connection accepted from peer
       */
      void connectionAcceptedEvent(pt::blit::PipeConnectionAccepted&);

      /**
       * Callback from InputPipeListener connection closed by peer
       */
      void connectionClosedByPeerEvent(pt::blit::PipeConnectionClosedByPeer&);

      /**
       * Callback from InputPipeListener connection reset by peer
       */
      void connectionResetByPeerEvent(pt::blit::PipeConnectionResetByPeer&);

      /**
       * Accept connections from FEROLs
       */
      void acceptConnections();

      /**
       * Drop all connections
       */
      void dropConnections();

      /**
       * Return all connected FEROL streams
       */
      void getActiveFerolStreams(typename Input<ReadoutUnit,Configuration>::FerolStreams&);

      /**
       * Start processing events
       */
      void startProcessing();

      /**
       * Drain the remainig events
       */
      void drain() const;

      /**
       * Stop processing events
       */
      void stopProcessing();


    private:

      void startPipesWorkLoop();
      bool processPipes(toolbox::task::WorkLoop*);
      void connectionGone(const int sid, const int pipeIndex, const std::string how);
      std::string getHostName(const std::string& ip);

      ReadoutUnit* readoutUnit_;

      volatile bool acceptConnections_;
      volatile bool doProcessing_;
      volatile bool processPipes_;
      volatile bool pipesActive_;

      pt::blit::PipeService* pipeService_;

      typedef std::vector<pt::blit::InputPipe*> InputPipes;
      InputPipes inputPipes_;

      typedef boost::shared_ptr< SocketStream<ReadoutUnit,Configuration> > SocketStreamPtr;
      typedef std::map<uint16_t,SocketStreamPtr> SocketStreams;
      SocketStreams socketStreams_;
      mutable boost::mutex mutex_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::FerolConnectionManager
(
  ReadoutUnit* readoutUnit
) :
  readoutUnit_(readoutUnit),
  acceptConnections_(false),
  doProcessing_(false),
  processPipes_(false),
  pipesActive_(false),
  pipeService_(0)
{
  try
  {
    pipeService_ = dynamic_cast<pt::blit::PipeService *>(pt::getPeerTransportAgent()->getPeerTransport("btcp","blit",pt::SenderReceiver));
  }
  catch (pt::exception::PeerTransportNotFound)
  {
    return;
  }

  startPipesWorkLoop();
  pipeService_->addPipeServiceListener(this);

  std::list<pt::blit::PipeAdvertisement> advs = pipeService_->getPipeAdvertisements("blit", "btcp");
  for (std::list<pt::blit::PipeAdvertisement>::iterator it = advs.begin(); it != advs.end(); ++it )
  {
    std::ostringstream msg;
    msg << "Creating input pipe for PSP index '" << (*it).getIndex() <<  "'";
    LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), msg.str());

    inputPipes_.push_back(pipeService_->createInputPipe((*it), this));
  }
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::~FerolConnectionManager()
{
  boost::mutex::scoped_lock sl(mutex_);
  processPipes_ = false;

  for ( InputPipes::iterator it = inputPipes_.begin(); it != inputPipes_.end(); ++it )
  {
    pipeService_->destroyInputPipe(*it);
  }
  inputPipes_.clear();
  socketStreams_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::pipeServiceEvent(pt::blit::PipeAdvertisement& adv)
{
  try
  {
    if ( doProcessing_ )
    {
      std::ostringstream msg;
      msg << "Received a PSP advertisement to create input pipe '" << adv.getIndex() << "' while processing events";
      XCEPT_RAISE(exception::TCP,msg.str());
    }

    boost::mutex::scoped_lock sl(mutex_);
    inputPipes_.push_back(pipeService_->createInputPipe(adv, this));
  }
  catch(xcept::Exception &e)
  {
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionAcceptedEvent(pt::blit::PipeConnectionAccepted& pca)
{
  try
  {
    const std::string peer = getHostName(pca.getPeerName());

    if ( !acceptConnections_ || doProcessing_ )
    {
      std::ostringstream msg;
      msg << "Received a connection from " << peer << ":" << pca.getPort() << " while ";
      if ( doProcessing_ )
        msg << "processing data";
      else
        msg << "not accepting new connections";
      XCEPT_RAISE(exception::TCP,msg.str());
    }

    const boost::shared_ptr<Configuration> configuration = readoutUnit_->getConfiguration();

    typename Configuration::FerolSources::const_iterator it = configuration->ferolSources.begin();
    const typename Configuration::FerolSources::const_iterator itEnd = configuration->ferolSources.end();
    bool found = false;

    while ( it != itEnd && !found )
    {
      if ( it->bag.hostname.value_ == peer && it->bag.port.value_ == pca.getPort() )
      {
        found = true;

        const uint16_t fedId = it->bag.fedId.value_;
        if (fedId > FED_COUNT)
        {
          std::ostringstream msg;
          msg << "The fedSourceId " << fedId;
          msg << " is larger than maximal value FED_COUNT=" << FED_COUNT;
          XCEPT_RAISE(exception::Configuration,msg.str());
        }

        const SocketStreamPtr socketStream(
          new SocketStream<ReadoutUnit,Configuration>(readoutUnit_,&it->bag)
        );

        {
          boost::mutex::scoped_lock sl(mutex_);
          socketStreams_.insert( typename SocketStreams::value_type(pca.getSID(),socketStream) );
        }

        std::ostringstream msg;
        msg << "Accepted connection from " << peer << ":" << pca.getPort();
        msg << " corresponding to FED id " << fedId;
        LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), msg.str());
      }
      else
      {
        ++it;
      }
    }

    if ( !found )
    {
      std::ostringstream msg;
      msg << "Received an unexpected connection from " << peer << ":" << pca.getPort();
      XCEPT_RAISE(exception::Configuration,msg.str());
    }
  }
  catch(xcept::Exception &e)
  {
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionClosedByPeerEvent(pt::blit::PipeConnectionClosedByPeer& pcc)
{
  connectionGone(pcc.getSID(), pcc.getPipeIndex(), "closed");
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionResetByPeerEvent(pt::blit::PipeConnectionResetByPeer& pcr)
{
  connectionGone(pcr.getSID(), pcr.getPipeIndex(), "reset");
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionGone(const int sid, const int pipeIndex, const std::string how)
{
  try
  {
    std::ostringstream msg;

    {
      boost::mutex::scoped_lock sl(mutex_);

      const typename SocketStreams::iterator pos = socketStreams_.find(sid);
      if ( pos != socketStreams_.end() )
      {
        const typename Configuration::FerolSource* ferolSource = pos->second->getFerolSource();
        msg << ferolSource->hostname.value_ << ":" << ferolSource->port.value_;
        msg << " (FED id " << ferolSource->fedId.value_ << ") ";
        msg << how << " the connection";
        socketStreams_.erase(pos);
      }
      else
      {
        msg << "Connection " << how << " from unknown peer with SID " << sid << " and pipe index " << pipeIndex;
      }
    }
    LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), msg.str());
  }
  catch(xcept::Exception &e)
  {
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::acceptConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(mutex_);
  acceptConnections_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::dropConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(mutex_);
  acceptConnections_ = false;

  socketStreams_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::getActiveFerolStreams
(
  typename Input<ReadoutUnit,Configuration>::FerolStreams& ferolStreams
)
{
  if ( ! pipeService_ )
  {
    XCEPT_RAISE(exception::Configuration, "Cannot get FEROL streams from BLIT because no pt::blit is available");
  }

  const boost::shared_ptr<Configuration> configuration = readoutUnit_->getConfiguration();

  std::set<uint32_t> fedIds;
  for ( xdata::Vector<xdata::UnsignedInteger32>::const_iterator it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
        it != itEnd; ++it )
  {
    fedIds.insert(it->value_);
  }

  const uint16_t maxTries = 100;
  uint16_t tries = 0;

  do
  {
    {
      boost::mutex::scoped_lock sl(mutex_);

      for ( typename SocketStreams::const_iterator it = socketStreams_.begin(), itEnd = socketStreams_.end();
            it != itEnd; ++it)
      {
        const uint32_t fedId = it->second->getFerolSource()->fedId.value_;
        if ( fedIds.erase(fedId) )
        {
          it->second->configure();
          ferolStreams.insert( typename Input<ReadoutUnit,Configuration>::FerolStreams::value_type(fedId,it->second) );
        }
      }
    }
    if ( ++tries > maxTries )
    {
      std::ostringstream msg;
      msg << "Missing connections from FEROLs for FED ids: ";
      std::copy(fedIds.begin(), fedIds.end(), std::ostream_iterator<uint32_t>(msg, " "));
      XCEPT_RAISE(exception::TCP, msg.str());
    }
    else if ( tries > 1 )
    {
      ::usleep(1000);
    }
  }
  while ( !fedIds.empty() );
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::startPipesWorkLoop()
{
  try
  {
    processPipes_ = true;

    toolbox::task::WorkLoop* pipesWorkLoop =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("pipes"), "waiting");

    if ( !pipesWorkLoop->isActive() )
      pipesWorkLoop->activate();

    toolbox::task::ActionSignature* pipesAction =
      toolbox::task::bind(this,
                          &evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::processPipes,
                          this->readoutUnit_->getIdentifier("pipesAction"));

    pipesWorkLoop->submit(pipesAction);
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start pipes workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::processPipes(toolbox::task::WorkLoop*)
{
  if ( ! processPipes_ ) return false;

  bool workDone;
  pipesActive_ = true;

  try
  {
    do
    {
      boost::mutex::scoped_lock sl(mutex_);

      workDone = false;

      for ( InputPipes::iterator it = inputPipes_.begin(); it != inputPipes_.end(); ++it )
      {
        pt::blit::BulkTransferEvent event;
        if ( (*it)->dequeue(&event) )
        {
          workDone = true;

          if ( readoutUnit_->getConfiguration()->dropAtSocket )
          {
            (*it)->grantBuffer(event.ref);
          }
          else
          {
            const typename SocketStreams::iterator pos = socketStreams_.find(event.sid);
            if ( pos != socketStreams_.end() )
            {
              pos->second->addBuffer(event.ref,*it);
            }
            else
            {
              (*it)->grantBuffer(event.ref);
            }
          }
        }
      }
    }
    while ( workDone );
  }
  catch(xcept::Exception &e)
  {
    pipesActive_ = false;
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    pipesActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    pipesActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  pipesActive_ = false;

  ::usleep(100);

  return processPipes_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::startProcessing()
{
  if ( ! pipeService_ ) return;

  doProcessing_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::drain() const
{
  while ( pipesActive_ ) ::usleep(1000);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::stopProcessing()
{
  doProcessing_ = false;
}


template<class ReadoutUnit,class Configuration>
std::string evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::getHostName(const std::string& ip)
{
  char host[256];
  char service[20];
  sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip.c_str());
  getnameinfo((sockaddr*)&address, sizeof(address), host, sizeof(host), service, sizeof(service), 0);
  return host;
}


#endif // _evb_readoutunit_FerolConnectionManager_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
