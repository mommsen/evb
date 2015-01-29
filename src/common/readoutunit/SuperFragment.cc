#include <sstream>

#include "evb/Exception.h"
#include "evb/readoutunit/SuperFragment.h"


evb::readoutunit::SuperFragment::SuperFragment(const EvBid& evbId)
  : evbId_(evbId),size_(0)
{}


void evb::readoutunit::SuperFragment::append(FedFragmentPtr& fedFragment)
{
  if ( fedFragment->getEvBid() != evbId_ )
  {
    missingFedIds_.push_back(fedFragment->getFedId());

    std::ostringstream oss;
    oss << "Mismatch detected: expected evb id "
      << evbId_ << ", but found evb id "
      << fedFragment->getEvBid() << " in data block from FED "
      << fedFragment->getFedId();
    XCEPT_RAISE(exception::MismatchDetected, oss.str());
  }

  if ( fedFragment->isCorrupted() )
  {
    missingFedIds_.push_back(fedFragment->getFedId());
  }
  else
  {
    fedFragments_.push_back(fedFragment);
    size_ += fedFragment->getFedSize();
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
