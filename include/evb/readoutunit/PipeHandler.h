#ifndef _evb_readoutunit_PipeHandler_h_
#define _evb_readoutunit_PipeHandler_h_

#include <map>
#include <set>
#include <stdint.h>
#include <string.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "evb/readoutunit/SocketBuffer.h"
#include "evb/readoutunit/SocketStream.h"
#include "pt/blit/InputPipe.h"
#include "pt/blit/PipeService.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Buffer.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xcept/tools.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Manage one pipe from pt::blit
    */

    template<class ReadoutUnit, class Configuration>
    class PipeHandler : public toolbox::lang::Class
    {
    public:

      PipeHandler(ReadoutUnit*, pt::blit::PipeService*, pt::blit::InputPipe*, const int index);

      ~PipeHandler();

      void createStream(const int sid, const typename Configuration::FerolSource*);

      void addFerolStreams(typename Input<ReadoutUnit,Configuration>::FerolStreams&, std::set<uint32_t>& fedIds);

      bool idle() const;

    private:

      void startPipeWorkLoop();
      bool processPipe(toolbox::task::WorkLoop*);
      void releaseBuffer(toolbox::mem::Reference*);

      ReadoutUnit* readoutUnit_;
      pt::blit::PipeService* pipeService_;
      pt::blit::InputPipe* inputPipe_;
      const int index_;

      toolbox::task::WorkLoop* pipeWorkLoop_;
      volatile bool processPipe_;
      volatile bool pipeActive_;
      int outstandingBuffers_;

      SocketBuffer::ReleaseFunction releaseFunction_;
      typedef OneToOneQueue<toolbox::mem::Reference*> GrantFIFO;
      GrantFIFO grantFIFO_;
      mutable boost::mutex grantFIFOmutex_;

      typedef boost::shared_ptr< SocketStream<ReadoutUnit,Configuration> > SocketStreamPtr;
      typedef std::map<uint16_t,SocketStreamPtr> SocketStreams;
      SocketStreams socketStreams_;
      mutable boost::mutex socketStreamsMutex_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::PipeHandler
(
  ReadoutUnit* readoutUnit,
  pt::blit::PipeService* pipeService,
  pt::blit::InputPipe* inputPipe,
  const int index
) :
  readoutUnit_(readoutUnit),
  pipeService_(pipeService),
  inputPipe_(inputPipe),
  index_(index),
  processPipe_(false),
  pipeActive_(false),
  outstandingBuffers_(0),
  grantFIFO_(readoutUnit,"grantFIFO_"+boost::lexical_cast<std::string>(index))
{
  releaseFunction_ = boost::bind(&evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::releaseBuffer, this, _1);
  grantFIFO_.resize(readoutUnit_->getConfiguration()->socketBufferFIFOCapacity);
  startPipeWorkLoop();
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::~PipeHandler()
{
  processPipe_ = false;
  pipeWorkLoop_->cancel();
  socketStreams_.clear();
  pipeService_->destroyInputPipe(inputPipe_);
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::idle() const
{
  while ( pipeActive_ || !grantFIFO_.empty() ) ::usleep(1000);
  std::cout << "*** " << index_ << "\t" << pipeActive_ << "\t" << outstandingBuffers_ << "\t" << grantFIFO_.elements() << std::endl;
  return ( outstandingBuffers_ == 0 );
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::createStream
(
  const int sid,
  const typename Configuration::FerolSource* ferolSource
)
{
  boost::mutex::scoped_lock sl(socketStreamsMutex_);

  const SocketStreamPtr socketStream(
    new SocketStream<ReadoutUnit,Configuration>(readoutUnit_,ferolSource)
  );
  socketStreams_.insert( typename SocketStreams::value_type(sid,socketStream) );
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::addFerolStreams
(
  typename Input<ReadoutUnit,Configuration>::FerolStreams& ferolStreams,
  std::set<uint32_t>& fedIds
)
{
  boost::mutex::scoped_lock sl(socketStreamsMutex_);

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


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::startPipeWorkLoop()
{
  const std::string identifier = readoutUnit_->getIdentifier();
  std::ostringstream workLoopName;
  workLoopName << identifier << "/Pipe_" << index_;

  try
  {
    processPipe_ = true;

    pipeWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(workLoopName.str(), "waiting");

    if ( !pipeWorkLoop_->isActive() )
      pipeWorkLoop_->activate();

    toolbox::task::ActionSignature* pipeAction =
      toolbox::task::bind(this,
                          &evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::processPipe,
                          this->readoutUnit_->getIdentifier("pipeAction"));

    pipeWorkLoop_->submit(pipeAction);
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start pipe workloop " + workLoopName.str();
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::processPipe(toolbox::task::WorkLoop*)
{
  if ( ! processPipe_ ) return false;

  bool workDone;
  pt::blit::BulkTransferEvent event;
  toolbox::mem::Reference* bufRef;

  pipeActive_ = true;

  try
  {
    do
    {
      workDone = false;

      if ( inputPipe_->dequeue(&event) )
      {
        SocketBufferPtr socketBuffer( new SocketBuffer(event.ref,releaseFunction_) );
        ++outstandingBuffers_;
        workDone = true;

        if ( ! readoutUnit_->getConfiguration()->dropAtSocket )
        {
          boost::mutex::scoped_lock sl(socketStreamsMutex_);
          const typename SocketStreams::iterator pos = socketStreams_.find(event.sid);
          if ( pos != socketStreams_.end() )
            pos->second->addBuffer(socketBuffer);
        }
      }

      if ( grantFIFO_.deq(bufRef) )
      {
        inputPipe_->grantBuffer(bufRef);
        --outstandingBuffers_;
        workDone = true;
      }
    }
    while ( workDone );
  }
  catch(xcept::Exception &e)
  {
    pipeActive_ = false;
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    pipeActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    pipeActive_ = false;
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  pipeActive_ = false;

  ::usleep(100);

  return processPipe_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::releaseBuffer(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(grantFIFOmutex_);

  grantFIFO_.enqWait(bufRef);
}


#endif // _evb_readoutunit_PipeHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
