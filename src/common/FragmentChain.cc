#include "interface/shared/i2ogevb2g.h"
#include "evb/FragmentChain.h"


evb::FragmentChain::FragmentChain() :
size_(0),
head_(0),tail_(0)
{}

evb::FragmentChain::FragmentChain(const uint32_t resourceCount) :
size_(0),
head_(0),tail_(0)
{
  for (uint32_t i = 1; i <= resourceCount; ++i)
  {
    resourceList_.push_back(i);
  }
}

evb::FragmentChain::FragmentChain(const EvBid& evbId, const ResourceList& resourceList) :
evbId_(evbId),
resourceList_(resourceList),
size_(0),
head_(0),tail_(0)
{}


evb::FragmentChain::~FragmentChain()
{
  if (head_) head_->release();
}


bool evb::FragmentChain::append
(
  const uint32_t resourceId,
  toolbox::mem::Reference* bufRef
)
{
  if ( ! checkResourceId(resourceId) ) return false;

  if (head_)
    tail_->setNextReference(bufRef);
  else
    head_ = bufRef;
  
  do {
    tail_ = bufRef;
    size_ += bufRef->getDataSize();
    bufRef = bufRef->getNextReference();
  } while (bufRef);
  
  return true;
}


bool evb::FragmentChain::checkResourceId(const uint32_t resourceId)
{
  ResourceList::iterator pos =
    std::find(resourceList_.begin(),resourceList_.end(),resourceId);

  if ( pos == resourceList_.end() ) return false;

  resourceList_.erase(pos);

  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
