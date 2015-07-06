#include "evb/readoutunit/SocketBuffer.h"


boost::mutex evb::readoutunit::SocketBuffer::mutex_;

evb::readoutunit::SocketBuffer::SocketBuffer(toolbox::mem::Reference* bufRef, pt::blit::InputPipe* inputPipe)
  : bufRef_(bufRef),inputPipe_(inputPipe)
{}


evb::readoutunit::SocketBuffer::~SocketBuffer()
{
  boost::mutex::scoped_lock sl(mutex_);
  inputPipe_->grantBuffer(bufRef_);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
