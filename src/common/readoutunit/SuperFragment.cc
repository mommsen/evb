#include <sstream>

#include "evb/Exception.h"
#include "evb/readoutunit/SuperFragment.h"


evb::readoutunit::SuperFragment::SuperFragment(const EvBid& evbId)
  : evbId_(evbId),size_(0)
{}


void evb::readoutunit::SuperFragment::discard(const FedFragmentPtr& fedFragment)
{
  missingFedIds_.push_back(fedFragment->getFedId());
}


bool evb::readoutunit::SuperFragment::append(const FedFragmentPtr& fedFragment)
{
  const EvBid& incomingEvBid = fedFragment->getEvBid();

  if ( incomingEvBid != evbId_ )
  {
    discard(fedFragment);

    std::ostringstream msg;
    msg << "Mismatch detected: expected evb id "
      << evbId_ << ", but found evb id "
      << incomingEvBid << " in data block from FED "
      << fedFragment->getFedId();
    XCEPT_RAISE(exception::MismatchDetected, msg.str());
  }

  fedFragments_.push_back(fedFragment);
  size_ += fedFragment->getFedSize();

  return ( (incomingEvBid.bxId() == evbId_.bxId()) ||
           // There's no agreement in CMS on how to label the last/first BX
           // TCDS calls it always 3564, but some subsystems call it 0.
           (incomingEvBid.bxId() == 0 && evbId_.bxId() == 3564) );
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
