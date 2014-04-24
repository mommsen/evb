#include "evb/Exception.h"
#include "evb/RU.h"
#include "evb/ru/RUinput.h"


bool evb::ru::RUinput::FEROLproxy::getSuperFragmentWithEvBid(const EvBid& evbId, readoutunit::FragmentChainPtr& superFragment)
{
  // if we got a superFragment from pt::frl, use it
  if ( superFragmentFIFO_.deq(superFragment) )
  {
    if ( superFragment->getEvBid() != evbId )
    {
      std::ostringstream oss;
      oss << "Mismatch detected: expected evb id "
        << evbId << ", but found evb id "
        << superFragment->getEvBid() << " in data block";
      XCEPT_RAISE(exception::MismatchDetected, oss.str());
    }

    return true;
  }

  if ( !fragmentFIFOs_.empty() && fragmentFIFOs_.begin()->second->empty() ) return false;

  readoutunit::FragmentChain::FragmentPtr fragment;
  superFragment.reset( new readoutunit::FragmentChain(evbId) );

  for (FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    while ( doProcessing_ && !it->second->deq(fragment) ) {};

    if ( ! doProcessing_ )
    {
      throw exception::HaltRequested();
    }

    if ( evbId != fragment->evbId )
    {
      std::ostringstream oss;
      oss << "Mismatch detected: expected evb id "
        << evbId << ", but found evb id "
        << fragment->evbId
        << " in data block from FED "
        << it->first;
      XCEPT_RAISE(exception::MismatchDetected, oss.str());
    }

    superFragment->append(fragment);
  }

  toolbox::mem::Reference* bufRef = 0;
  if ( getScalerFragment(evbId,bufRef) )
    superFragment->append(bufRef);

  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
