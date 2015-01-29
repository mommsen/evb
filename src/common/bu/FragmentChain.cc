#include "evb/bu/FragmentChain.h"
#include "evb/I2OMessages.h"


evb::bu::FragmentChain::FragmentChain() :
  head_(0),tail_(0),
  size_(0)
{}


evb::bu::FragmentChain::FragmentChain(const uint32_t resourceCount) :
  head_(0),tail_(0),
  size_(0)
{
  for (uint32_t i = 1; i <= resourceCount; ++i)
  {
    resourceList_.push_back(i);
  }
}


evb::bu::FragmentChain::~FragmentChain()
{
  for ( BufRefs::iterator it = bufRefs_.begin(), itEnd = bufRefs_.end();
        it != itEnd; ++it )
  {
    // break any chain
    (*it)->setNextReference(0);
    (*it)->release();
  }
  bufRefs_.clear();
}


bool evb::bu::FragmentChain::append
(
  const uint16_t resourceId,
  toolbox::mem::Reference* bufRef
)
{
  if ( ! checkResourceId(resourceId) ) return false;

  append(bufRef);

  return true;
}


void evb::bu::FragmentChain::append
(
  toolbox::mem::Reference* bufRef
)
{
  chainFragment(bufRef);

  do
  {
    bufRefs_.push_back(bufRef);
    bufRef = bufRef->getNextReference();
  }
  while (bufRef);
}


bool evb::bu::FragmentChain::checkResourceId(const uint16_t resourceId)
{
  ResourceList::iterator pos =
    std::find(resourceList_.begin(),resourceList_.end(),resourceId);

  if ( pos == resourceList_.end() ) return false;

  resourceList_.erase(pos);

  return true;
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
