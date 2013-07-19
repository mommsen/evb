#include "evb/RU.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/States.h"
#include "evb/ru/RUinput.h"


evb::RU::RU(xdaq::ApplicationStub* app) :
ru::ReadoutUnit(app,"/evb/images/ru64x64.gif")
{
  this->stateMachine_.reset( new ru::RUStateMachine(this) );
  this->input_.reset( new ru::RUinput(app,this->configuration_) );
  this->buProxy_.reset( new readoutunit::BUproxy<RU>(this) );

  this->initialize();

  LOG4CPLUS_INFO(this->logger_, "End of constructor");
}


namespace evb {
  namespace readoutunit {

    template<>
    void BUproxy<RU>::fillRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest)
    {
      readoutMsg->getEvBids(fragmentRequest->evbIds);
      readoutMsg->getRUtids(fragmentRequest->ruTids);
    }

    template<>
    bool BUproxy<RU>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      if ( ! fragmentRequestFIFO_.deq(fragmentRequest) ) return false;

      for (uint32_t i=0; i < fragmentRequest->nbRequests; ++i)
      {
        const EvBid& evbId = fragmentRequest->evbIds.at(i);
        FragmentChainPtr superFragment;

        while ( doProcessing_ &&
          !input_->getSuperFragmentWithEvBid(evbId, superFragment) ) ::usleep(10);

        if ( superFragment->isValid() ) superFragments.push_back(superFragment);
      }

      return !superFragments.empty();
    }
  }
}


/**
 * Provides the factory method for the instantiation of RU applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::RU)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
