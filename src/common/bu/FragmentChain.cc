#include "evb/bu/FragmentChain.h"
#include "evb/I2OMessages.h"


evb::bu::FragmentChain::FragmentChain(uint32_t blockCount) :
  blockCount_(blockCount),
  head_(0),tail_(0),
  size_(0)
{}


evb::bu::FragmentChain::~FragmentChain()
{
  if ( head_ ) head_->release();
}


bool evb::bu::FragmentChain::append
(
  toolbox::mem::Reference* bufRef
)
{
  chainFragment(bufRef);
  return ( --blockCount_ == 0 );
}


void evb::bu::FragmentChain::chainFragment
(
  toolbox::mem::Reference* bufRef
)
{
  if ( ! head_ )
    head_ = bufRef;
  else
    tail_->setNextReference(bufRef);

  do
  {
    size_ += bufRef->getDataSize() - sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME);
    tail_ = bufRef;
    bufRef = bufRef->getNextReference();
  }
  while (bufRef);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
