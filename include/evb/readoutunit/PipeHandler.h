#ifndef _evb_readoutunit_PipeHandler_h_
#define _evb_readoutunit_PipeHandler_h_

#include <map>
#include <set>
#include <stdint.h>
#include <string.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "evb/readoutunit/SocketStream.h"
#include "pt/blit/InputPipe.h"
#include "pt/blit/PipeService.h"
#include "toolbox/lang/Class.h"
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

      void closeConnection(const int sid, const std::string how);

      void addFerolStreams(typename Input<ReadoutUnit,Configuration>::FerolStreams&, std::set<uint32_t>& fedIds);


    private:

      void startPipeWorkLoop(const int index);
      bool processPipe(toolbox::task::WorkLoop*);

      ReadoutUnit* readoutUnit_;
      pt::blit::PipeService* pipeService_;
      pt::blit::InputPipe* inputPipe_;
      toolbox::task::WorkLoop* pipeWorkLoop_;
      volatile bool processPipe_;

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
  processPipe_(false)
{
  startPipeWorkLoop(index);
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
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::closeConnection(const int sid, const std::string how)
{
  boost::mutex::scoped_lock sl(socketStreamsMutex_);

  std::ostringstream msg;

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
    msg << "Connection " << how << " from unknown peer with SID " << sid;
  }
  LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), msg.str());
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
void evb::readoutunit::PipeHandler<ReadoutUnit,Configuration>::startPipeWorkLoop(const int index)
{
  const std::string identifier = readoutUnit_->getIdentifier();
  std::ostringstream workLoopName;
  workLoopName << identifier << "/Pipe_" << index;

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

  try
  {
    do
    {
      workDone = false;

      if ( inputPipe_->dequeue(&event) )
      {
        workDone = true;

        if ( readoutUnit_->getConfiguration()->dropAtSocket )
        {
          inputPipe_->grantBuffer(event.ref);
        }
        else
        {
          boost::mutex::scoped_lock sl(socketStreamsMutex_);
          const typename SocketStreams::iterator pos = socketStreams_.find(event.sid);
          if ( pos != socketStreams_.end() )
          {
            pos->second->addBuffer(event.ref,inputPipe_);
          }
          else
          {
            inputPipe_->grantBuffer(event.ref);
          }
        }
      }
    }
    while ( processPipe_ && workDone );
  }
  catch(xcept::Exception &e)
  {
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    XCEPT_DECLARE(exception::DummyData,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  ::usleep(100);

  return processPipe_;
}


#endif // _evb_readoutunit_PipeHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
