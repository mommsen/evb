#include "evb/RU.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/States.h"


evb::RU::RU(xdaq::ApplicationStub* app) :
  ru::ReadoutUnit(app,"/evb/images/ru64x64.gif")
{
  this->stateMachine_.reset( new ru::RUStateMachine(this) );
  this->input_.reset( new readoutunit::Input<RU,readoutunit::Configuration>(this) );
  this->buProxy_.reset( new readoutunit::BUproxy<RU>(this) );

  this->initialize();

  LOG4CPLUS_INFO(this->getApplicationLogger(), "End of constructor");
}


namespace evb {
  namespace readoutunit {

    template<>
    void BUproxy<RU>::fillRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest) const
    {
      readoutMsg->getEvBids(fragmentRequest->evbIds);
      readoutMsg->getRUtids(fragmentRequest->ruTids);
    }

    template<>
    bool BUproxy<RU>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      if ( ! fragmentRequestFIFO_.deq(fragmentRequest) ) return false;

      try
      {
        for (uint32_t i=0; i < fragmentRequest->nbRequests; ++i)
        {
          const EvBid& evbId = fragmentRequest->evbIds.at(i);
          FragmentChainPtr superFragment;

          while ( ! input_->getSuperFragmentWithEvBid(evbId, superFragment) )
          {
            if ( ! doProcessing_ ) return false;
            ::usleep(10);
          }

          superFragments.push_back(superFragment);
        }
      }
      catch(exception::HaltRequested)
      {
        return false;
      }

      return true;
    }


    template<>
    bool BUproxy<RU>::isEmpty()
    {
      boost::mutex::scoped_lock sl(processesActiveMutex_);
      return ( fragmentRequestFIFO_.empty() && processesActive_.none() );
    }


    template<>
    std::string BUproxy<RU>::getHelpTextForBuRequests() const
    {
      return "BU event requests forwarded by the EVM. If no events are requested, the EVM has not got any requests or has no data.";
    }


    template<>
    void ru::ReadoutUnit::addComponentsToWebPage
    (
      cgicc::table& table
    ) const
    {
      using namespace cgicc;

      table.add(tr()
                .add(td(input_->getHtmlSnipped()).set("class","xdaq-evb-component"))
                .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
                .add(td(buProxy_->getHtmlSnipped()).set("class","xdaq-evb-component")));
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
