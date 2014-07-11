#include "evb/Exception.h"
#include "evb/RU.h"
#include "evb/ru/RUinput.h"


bool evb::ru::RUinput::FEROLproxy::getSuperFragmentWithEvBid(const EvBid& referenceEvBid, readoutunit::FragmentChainPtr& superFragment)
{
  if ( !fragmentFIFOs_.empty() && fragmentFIFOs_.begin()->second->empty() ) return false;

  FedFragmentPtr fragment;
  superFragment.reset( new readoutunit::FragmentChain(referenceEvBid) );

  for (FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    while ( doProcessing_ && !it->second->deq(fragment) ) {};

    if ( ! doProcessing_ )
    {
      throw exception::HaltRequested();
    }

    const EvBid evbId = evbIdFactories_[fragment->getFedId()].getEvBid(fragment->getEventNumber());
    superFragment->append(evbId,fragment);
  }

  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
