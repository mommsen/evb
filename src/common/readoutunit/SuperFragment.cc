#include <sstream>

#include "evb/Exception.h"
#include "evb/readoutunit/SuperFragment.h"


evb::readoutunit::SuperFragment::SuperFragment(const EvBid& evbId)
  : evbId_(evbId),size_(0)
{}


bool evb::readoutunit::SuperFragment::append(const FedFragmentPtr& fedFragment)
{
  if ( fedFragment->getEvBid() != evbId_ )
  {
    missingFedIds_.push_back(fedFragment->getFedId());

    std::ostringstream msg;
    msg << "Mismatch detected: expected evb id "
      << evbId_ << ", but found evb id "
      << fedFragment->getEvBid() << " in data block from FED "
      << fedFragment->getFedId();
    XCEPT_RAISE(exception::MismatchDetected, msg.str());
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

  return ( (fedFragment->getEvBid().bxId() == evbId_.bxId()) ||
           // There's no agreement in CMS on how to label the last/first BX
           (fedFragment->getEvBid().bxId() == 0 && evbId_.bxId() == 3564) );
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
