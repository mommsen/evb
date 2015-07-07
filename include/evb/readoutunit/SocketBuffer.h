#ifndef _evb_readoutunit_SocketBuffer_h_
#define _evb_readoutunit_SocketBuffer_h_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

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

      typedef boost::function< void(toolbox::mem::Reference*) > ReleaseFunction;

      SocketBuffer(toolbox::mem::Reference* bufRef, ReleaseFunction& releaseFunction)
        : bufRef_(bufRef),releaseFunction_(releaseFunction) {}

      ~SocketBuffer()
      { releaseFunction_(bufRef_); }

      toolbox::mem::Reference* getBufRef() const
      { return bufRef_; }

    private:

      toolbox::mem::Reference* bufRef_;
      const ReleaseFunction& releaseFunction_;

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
