#ifndef _evb_readoutunit_SocketBuffer_h_
#define _evb_readoutunit_SocketBuffer_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#include "pt/blit/InputPipe.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Represent a Socket buffer
     */

    class SocketBuffer
    {
    public:

      SocketBuffer(toolbox::mem::Reference* bufRef, pt::blit::InputPipe* inputPipe)
        : bufRef_(bufRef),inputPipe_(inputPipe) {}

      ~SocketBuffer()
      { inputPipe_->grantBuffer(bufRef_); }

      toolbox::mem::Reference* getBufRef() const
      { return bufRef_; }

    private:

      toolbox::mem::Reference* bufRef_;
      pt::blit::InputPipe* inputPipe_;

    };

    typedef boost::shared_ptr<SocketBuffer> SocketBufferPtr;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_SocketBuffer_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
