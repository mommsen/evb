#ifndef _evb_readoutunit_SocketStream_h_
#define _evb_readoutunit_SocketStream_h_

#include <mutex>
#include <stdint.h>
#include <string.h>

#include "evb/OneToOneQueue.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/SocketBuffer.h"
#include "evb/readoutunit/StateMachine.h"
#include "interface/shared/ferol_header.h"
#include "pt/blit/InputPipe.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Represent a stream of locally generated FEROL data
    */

    template<class ReadoutUnit, class Configuration>
    class SocketStream : public FerolStream<ReadoutUnit,Configuration>, public toolbox::lang::Class
    {
    public:

      SocketStream(ReadoutUnit*, const uint16_t fedId);

      ~SocketStream();

      /**
       * Handle the next buffer from pt::blit
       */
      void addBuffer(SocketBufferPtr&);

      /**
       * Configure
       */
      void configure();

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);

      /**
       * Drain the remainig events
       */
      virtual void drain();

      /**
       * Stop processing events
       */
      virtual void stopProcessing();


    private:

      void startParseSocketBuffersWorkLoop();
      bool parseSocketBuffers(toolbox::task::WorkLoop*);

      using SocketBufferFIFO = OneToOneQueue<SocketBufferPtr>;
      SocketBufferFIFO socketBufferFIFO_;

      toolbox::task::WorkLoop* parseSocketBuffersWL_;
      toolbox::task::ActionSignature* parseSocketBuffersAction_;

      volatile bool parseSocketBuffersActive_;

      FedFragmentPtr currentFragment_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::SocketStream
(
  ReadoutUnit* readoutUnit,
  const uint16_t fedId
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,fedId),
  socketBufferFIFO_(readoutUnit,"socketBufferFIFO_FED_"+boost::lexical_cast<std::string>(fedId)),
  parseSocketBuffersActive_(false)
{
  startParseSocketBuffersWorkLoop();
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::~SocketStream()
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startParseSocketBuffersWorkLoop()
{
  try
  {
    const std::string fedIdStr = boost::lexical_cast<std::string>(this->fedId_);

    parseSocketBuffersWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( this->readoutUnit_->getIdentifier("parseSocketBuffers_"+fedIdStr), "waiting" );

    parseSocketBuffersAction_ =
      toolbox::task::bind(this, &evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::parseSocketBuffers,
                          this->readoutUnit_->getIdentifier("parseSocketBuffersAction_"+fedIdStr) );

    if ( ! parseSocketBuffersWL_->isActive() )
      parseSocketBuffersWL_->activate();
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'parseSocketBuffers'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::addBuffer(SocketBufferPtr& socketBuffer)
{
  if ( this->doProcessing_ )
    socketBufferFIFO_.enqWait(socketBuffer,this->doProcessing_);
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::parseSocketBuffers(toolbox::task::WorkLoop* wl)
{
  if ( ! this->doProcessing_ ) return false;

  parseSocketBuffersActive_ = true;

  try
  {
    SocketBufferPtr socketBuffer;
    while ( this->doProcessing_ && socketBufferFIFO_.deq(socketBuffer) )
    {
      const uint32_t usedBufferSize = socketBuffer->getBufRef()->getDataSize();
      {
        std::lock_guard<std::mutex> guard(this->socketMonitorMutex_);

        ++(this->socketMonitor_.perf.logicalCount);
        this->socketMonitor_.perf.sumOfSizes += usedBufferSize;
        this->socketMonitor_.perf.sumOfSquares += usedBufferSize*usedBufferSize;
      }
      uint32_t usedSize = 0;

      while ( this->doProcessing_ && usedSize < usedBufferSize )
      {
        if ( ! currentFragment_ )
          currentFragment_ = this->fedFragmentFactory_.getFedFragment(this->fedId_,this->isMasterStream_);

        if ( this->fedFragmentFactory_.append(currentFragment_,socketBuffer,usedSize) )
        {
          this->addFedFragment(currentFragment_);
          currentFragment_.reset();
        }
      }
    }
  }
  catch(xcept::Exception& e)
  {
    parseSocketBuffersActive_ = false;
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    parseSocketBuffersActive_ = false;
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, e.what());
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    parseSocketBuffersActive_ = false;
    XCEPT_DECLARE(exception::TCP,
                  sentinelException, "unkown exception");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  parseSocketBuffersActive_ = false;

  ::usleep(100);

  return this->doProcessing_;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::configure()
{
  socketBufferFIFO_.resize(this->readoutUnit_->getConfiguration()->socketBufferFIFOCapacity);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);
  parseSocketBuffersWL_->submit(parseSocketBuffersAction_);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::drain()
{
  while ( parseSocketBuffersActive_ || !socketBufferFIFO_.empty() ) ::usleep(1000);
  FerolStream<ReadoutUnit,Configuration>::drain();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::stopProcessing()
{
  FerolStream<ReadoutUnit,Configuration>::stopProcessing();
  while (parseSocketBuffersActive_) ::usleep(1000);
  socketBufferFIFO_.clear();
  currentFragment_.reset();
  this->fragmentFIFO_.clear();
}

#endif // _evb_readoutunit_SocketStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
