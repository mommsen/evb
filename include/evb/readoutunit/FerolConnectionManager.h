#ifndef _evb_readoutunit_FerolConnectionManager_h_
#define _evb_readoutunit_FerolConnectionManager_h_

#include <netdb.h>
#include <set>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/PipeHandler.h"
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
    class FerolConnectionManager : public pt::blit::PipeServiceListener, public pt::blit::InputPipeListener
    {
    public:

      FerolConnectionManager(ReadoutUnit*);

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

      void createPipes();
      bool handlersIdle() const;
      std::string getHostName(const std::string& ip);

      ReadoutUnit* readoutUnit_;

      volatile bool acceptConnections_;

      pt::blit::PipeService* pipeService_;

      typedef boost::shared_ptr< PipeHandler<ReadoutUnit,Configuration> > PipeHandlerPtr;
      typedef std::map<uint16_t,PipeHandlerPtr> PipeHandlers;
      PipeHandlers pipeHandlers_;
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

  pipeService_->addPipeServiceListener(this);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::pipeServiceEvent(pt::blit::PipeAdvertisement& adv)
{
  try
  {
    boost::mutex::scoped_lock sl(mutex_);

    if ( ! acceptConnections_ )
    {
      std::ostringstream msg;
      msg << "Received a PSP advertisement to create input pipe '" << adv.getIndex() << "' while not accepting connections";
      XCEPT_RAISE(exception::TCP,msg.str());
    }

    pt::blit::InputPipe* inputPipe = pipeService_->createInputPipe(adv, this);
    const PipeHandlerPtr pipeHandler(
      new PipeHandler<ReadoutUnit,Configuration>(readoutUnit_,pipeService_,inputPipe,adv.toString())
    );
    if ( ! pipeHandlers_.insert(typename PipeHandlers::value_type(adv.getIndex(),pipeHandler)).second )
    {
      std::ostringstream msg;
      msg << "Received a PSP advertisement to create input pipe with index '" << adv.getIndex() << "' which already exists";
      XCEPT_RAISE(exception::TCP,msg.str());
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
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionAcceptedEvent(pt::blit::PipeConnectionAccepted& pca)
{
  try
  {
    boost::mutex::scoped_lock sl(mutex_);

    const std::string peer = getHostName(pca.getPeerName());

    if ( !acceptConnections_ )
    {
      std::ostringstream msg;
      msg << "Received a connection from " << peer << ":" << pca.getPort();
      msg << " while not accepting new connections";
      XCEPT_RAISE(exception::TCP,msg.str());
    }

    const typename PipeHandlers::iterator handler = pipeHandlers_.find( pca.getPipeIndex() );
    if ( handler == pipeHandlers_.end() )
    {
      std::ostringstream msg;
      msg << "Received a connection from " << peer << ":" << pca.getPort();
      msg << " on pipe '" <<  pca.getPipeIndex() << "' for which there is no handler";
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

        handler->second->createStream(pca.getSID(),&it->bag);

        std::ostringstream msg;
        msg << "Accepted connection from " << peer << ":" << pca.getPort();
        msg << " with SID " << pca.getSID();
        msg << " and pipe index " << pca.getPipeIndex();
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
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionResetByPeerEvent(pt::blit::PipeConnectionResetByPeer& pcr)
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::acceptConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(mutex_);
  createPipes();
  acceptConnections_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::dropConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(mutex_);
  acceptConnections_ = false;
  pipeHandlers_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::createPipes()
{
  std::list<pt::blit::PipeAdvertisement> advs = pipeService_->getPipeAdvertisements("blit", "btcp");
  for (std::list<pt::blit::PipeAdvertisement>::iterator it = advs.begin(); it != advs.end(); ++it )
  {
    pt::blit::InputPipe* inputPipe = pipeService_->createInputPipe((*it), this);
    const PipeHandlerPtr pipeHandler(
      new PipeHandler<ReadoutUnit,Configuration>(readoutUnit_,pipeService_,inputPipe,it->toString())
    );
    if ( pipeHandlers_.insert(typename PipeHandlers::value_type(it->getIndex(),pipeHandler)).second )
    {
      std::ostringstream msg;
      msg << "Creating input pipe for PSP index '" << it->getIndex() <<  "'";
      LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), msg.str());
    }
    else
    {
      std::ostringstream msg;
      msg << "Received a PSP advertisement to create input pipe with index '" << it->getIndex() << "' which already exists";
      XCEPT_RAISE(exception::TCP,msg.str());
    }
  }
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

  const uint16_t maxTries = configuration->ferolConnectTimeOut*1000;
  uint16_t tries = 0;

  do
  {
    {
      boost::mutex::scoped_lock sl(mutex_);

      for ( typename PipeHandlers::const_iterator it = pipeHandlers_.begin(), itEnd = pipeHandlers_.end();
            it != itEnd; ++it)
      {
        it->second->addFerolStreams(ferolStreams,fedIds);
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
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::startProcessing()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(mutex_);

  acceptConnections_ = false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::drain() const
{
  bool idle = handlersIdle();

  while ( ! idle )
  {
    ::usleep(1000);
    idle = handlersIdle();
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::stopProcessing()
{
  if ( ! handlersIdle() )
  {
    XCEPT_RAISE(exception::TCP,"Failed to free all pt::blit buffers");
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::handlersIdle() const
{
  boost::mutex::scoped_lock sl(mutex_);

  for ( typename PipeHandlers::const_iterator it = pipeHandlers_.begin(), itEnd = pipeHandlers_.end();
        it != itEnd; ++it)
  {
    if ( ! it->second->idle() ) return false;
  }
  return true;
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
