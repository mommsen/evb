#ifndef _evb_readoutunit_FerolConnectionManager_h_
#define _evb_readoutunit_FerolConnectionManager_h_

#include <stdint.h>
#include <string.h>
#include <vector>

#include "evb/readoutunit/Input.h"
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
      void getAllFerolStreams(typename Input<ReadoutUnit,Configuration>::FerolStreams&);

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

      ReadoutUnit* readoutUnit_;

      toolbox::task::WorkLoop* pipesWorkLoop_;
      toolbox::task::ActionSignature* pipesAction_;

      volatile bool pipesActive_;
      volatile bool doProcessing_;

      pt::blit::PipeService* pipeService_;

      typedef std::vector<pt::blit::InputPipe*> InputPipes;
      InputPipes inputPipes_;
      mutable boost::mutex inputPipesMutex_;

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
  pipesActive_(false),
  doProcessing_(false),
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
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::pipeServiceEvent(pt::blit::PipeAdvertisement& adv)
{
  if ( doProcessing_ )
  {
    std::ostringstream msg;
    msg << "Received a PSP advertisement to create input pipe'" << adv.getIndex() << "' while processing events";
    XCEPT_RAISE(exception::TCP, msg.str());
  }

  boost::mutex::scoped_lock sl(inputPipesMutex_);
  inputPipes_.push_back(pipeService_->createInputPipe(adv, this));
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionAcceptedEvent(pt::blit::PipeConnectionAccepted& pca)
{
  std::cout << "connection accepted from peer: " << pca.getPeerName() << " port:" << pca.getPort() << " sid:" << pca.getSID() << " index: " << pca.getPipeIndex() <<  std::endl;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::connectionClosedByPeerEvent(pt::blit::PipeConnectionClosedByPeer& pca)
{
  std::cout << "connection closed by peer:"  << " sid:" << pca.getSID() << " index: " << pca.getPipeIndex() <<  std::endl;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::acceptConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(inputPipesMutex_);

  std::list<pt::blit::PipeAdvertisement> advs = pipeService_->getPipeAdvertisements("blit", "btcp");
  for (std::list<pt::blit::PipeAdvertisement>::iterator it = advs.begin(); it != advs.end(); ++it )
  {
    std::cout << " creating input pipe for PSP index '" << (*it).getIndex() <<  "'" << std::endl;
    inputPipes_.push_back(pipeService_->createInputPipe((*it), this));
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::dropConnections()
{
  if ( ! pipeService_ ) return;

  boost::mutex::scoped_lock sl(inputPipesMutex_);

  for ( InputPipes::iterator it = inputPipes_.begin(); it != inputPipes_.end(); ++it )
  {
    pipeService_->destroyInputPipe(*it);
  }

  inputPipes_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::getAllFerolStreams
(
  typename Input<ReadoutUnit,Configuration>::FerolStreams& ferolStreams
)
{
  if ( ! pipeService_ )
  {
    XCEPT_RAISE(exception::Configuration, "Cannot get FEROL streams from BLIT because no pt::blit is available");
  }

  ferolStreams.clear();

  const boost::shared_ptr<Configuration> configuration = readoutUnit_->getConfiguration();

  typename Configuration::FerolSources::const_iterator it, itEnd;
  for (it = configuration->ferolSources.begin(),
         itEnd = configuration->ferolSources.end(); it != itEnd; ++it)
  {
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::startPipesWorkLoop()
{
  try
  {
    pipesWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("pipes"), "waiting");

    if ( !pipesWorkLoop_->isActive() )
      pipesWorkLoop_->activate();

    pipesAction_ =
      toolbox::task::bind(this,
                          &evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::processPipes,
                          this->readoutUnit_->getIdentifier("pipesAction"));
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
  if ( ! doProcessing_ ) return false;

  boost::mutex::scoped_lock sl(inputPipesMutex_);

  pipesActive_ = true;
  bool workDone;

  try
  {
    do
    {
      workDone = false;

      for ( InputPipes::iterator it = inputPipes_.begin(); it != inputPipes_.end(); ++it )
      {
        pt::blit::BulkTransferEvent event;
        if ( (*it)->dequeue(&event) )
        {
          workDone = true;

          if ( readoutUnit_->getConfiguration()->dropAtSocket )
            (*it)->grantBuffer(event.ref);
        }
      }
    }
    while ( workDone && doProcessing_ );
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

  return doProcessing_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolConnectionManager<ReadoutUnit,Configuration>::startProcessing()
{
  if ( ! pipeService_ ) return;

  doProcessing_ = true;
  pipesWorkLoop_->submit(pipesAction_);
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


#endif // _evb_readoutunit_FerolConnectionManager_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
