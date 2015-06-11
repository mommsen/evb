#ifndef _evb_readoutunit_SocketStream_h_
#define _evb_readoutunit_SocketStream_h_

#include <stdint.h>
#include <string.h>

#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/SocketBuffer.h"
#include "evb/readoutunit/StateMachine.h"
#include "interface/shared/ferol_header.h"
#include "pt/blit/InputPipe.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
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
      void addBuffer(toolbox::mem::Reference*, pt::blit::InputPipe*);

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


    private:

      const typename Configuration::FerolSource* ferolSource_;
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
  ferolSource_(ferolSource)
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::addBuffer(toolbox::mem::Reference* bufRef, pt::blit::InputPipe* inputPipe)
{
  SocketBufferPtr socketBuffer( new SocketBuffer(bufRef,inputPipe) );

  // todo: split this into a separate thread

  uint32_t usedSize = 0;

  while ( usedSize < bufRef->getDataSize() )
  {
    if ( ! currentFragment_ )
      currentFragment_ = this->fedFragmentFactory_.getFedFragment();
    this->fedFragmentFactory_.append(currentFragment_,socketBuffer,usedSize);

    if ( currentFragment_->isComplete() )
    {
      this->addFedFragment(currentFragment_);
      currentFragment_.reset();
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::drain()
{
  FerolStream<ReadoutUnit,Configuration>::drain();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::SocketStream<ReadoutUnit,Configuration>::stopProcessing()
{
  FerolStream<ReadoutUnit,Configuration>::stopProcessing();
}


#endif // _evb_readoutunit_SocketStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
