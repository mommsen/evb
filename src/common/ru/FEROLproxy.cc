#include "evb/ru/RUinput.h"


bool evb::ru::RUinput::FEROLproxy::getSuperFragmentWithEvBid(const EvBid& evbId, readoutunit::FragmentChainPtr& superFragment)
{
  if ( ! superFragmentFIFO_.deq(superFragment) ) return false;

  if ( superFragment->getEvBid() != evbId )
  {
    std::stringstream oss;

    oss << "Mismatch detected: expected evb id "
      << evbId << ", but found evb id "
      << superFragment->getEvBid() << " in data block.";

    XCEPT_RAISE(exception::MismatchDetected, oss.str());
  }

  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
