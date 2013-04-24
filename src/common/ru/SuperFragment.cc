#include "interface/shared/i2ogevb2g.h"
#include "evb/ru/SuperFragment.h"


evb::ru::SuperFragment::SuperFragment() :
size_(0),
head_(0),tail_(0)
{}

evb::ru::SuperFragment::SuperFragment(const EvBid& evbId, const FEDlist& fedList) :
evbId_(evbId),
remainingFEDs_(fedList),
size_(0),
head_(0),tail_(0)
{}


evb::ru::SuperFragment::~SuperFragment()
{
  if (head_) head_->release();
}


bool evb::ru::SuperFragment::append
(
  const uint16_t fedId,
  toolbox::mem::Reference* bufRef
)
{
  if ( ! checkFedId(fedId) ) return false;

  if (head_)
    tail_->setNextReference(bufRef);
  else
    head_ = bufRef;
  
  toolbox::mem::Reference* nextBufRef = bufRef;
  while (nextBufRef)
  {
    tail_ = nextBufRef;
    size_ += nextBufRef->getDataSize() - sizeof(I2O_DATA_READY_MESSAGE_FRAME);
    nextBufRef = bufRef->getNextReference();
  }

  return true;
}


bool evb::ru::SuperFragment::checkFedId(const uint16_t fedId)
{
  FEDlist::iterator fedPos =
    std::find(remainingFEDs_.begin(),remainingFEDs_.end(),fedId);

  if ( fedPos == remainingFEDs_.end() ) return false;

  remainingFEDs_.erase(fedPos);

  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
