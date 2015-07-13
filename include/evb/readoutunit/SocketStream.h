#ifndef _evb_readoutunit_SocketStream_h_
#define _evb_readoutunit_SocketStream_h_

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

      SocketStream(ReadoutUnit*, const typename Configuration::FerolSource*);

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

      /**
       * Return the information about the FEROL connected to this stream
       */
      const typename Configuration::FerolSource* getFerolSource() const
      { return ferolSource_; }

      /**
       * Return the content of the socket and fragment FIFO as HTML snipped
       */
      virtual cgicc::div getHtmlSnippedForFragmentFIFO() const;


    private:

      void startParseSocketBuffersWorkLoop();
      bool parseSocketBuffers(toolbox::task::WorkLoop*);

      const typename Configuration::FerolSource* ferolSource_;

      typedef OneToOneQueue<SocketBufferPtr> SocketBufferFIFO;
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
  const typename Configuration::FerolSource* ferolSource
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,ferolSource->fedId.value_),
  ferolSource_(ferolSource),
  socketBufferFIFO_(readoutUnit,"socketBufferFIFO_FED_"+boost::lexical_cast<std::string>(ferolSource->fedId.value_)),
  parseSocketBuffersActive_(false)
{
  startParseSocketBuffersWorkLoop();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startParseSocketBuffersWorkLoop()
{
  try
  {
    const std::string fedIdStr = boost::lexical_cast<std::string>(ferolSource_->fedId.value_);

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
    socketBufferFIFO_.enqWait(socketBuffer);
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
      const uint32_t bufSize = socketBuffer->getBufRef()->getDataSize();
      uint32_t usedSize = 0;

      while ( this->doProcessing_ && usedSize < bufSize )
      {
        if ( ! currentFragment_ )
          currentFragment_ = this->fedFragmentFactory_.getFedFragment();

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
  socketBufferFIFO_.clear();
  currentFragment_.reset();
  this->fragmentFIFO_.clear();
}


template<class ReadoutUnit,class Configuration>
cgicc::div evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::getHtmlSnippedForFragmentFIFO() const
{
  cgicc::div div;
  div.add( socketBufferFIFO_.getHtmlSnipped() );
  div.add( this->fragmentFIFO_.getHtmlSnipped() );
  return div;
}

#endif // _evb_readoutunit_SocketStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -