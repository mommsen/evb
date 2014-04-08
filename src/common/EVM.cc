#include <sched.h>

#include "evb/EVM.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/States.h"
#include "evb/evm/EVMinput.h"
#include "xgi/Method.h"


evb::EVM::EVM(xdaq::ApplicationStub* app) :
evm::ReadoutUnit(app,"/evb/images/evm64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  this->stateMachine_.reset( new evm::EVMStateMachine(this) );
  this->input_.reset( new evm::EVMinput(this) );
  this->ruProxy_.reset( new evm::RUproxy(this,fastCtrlMsgPool) );
  this->buProxy_.reset( new readoutunit::BUproxy<EVM>(this) );

  this->initialize();

  LOG4CPLUS_INFO(this->getApplicationLogger(), "End of constructor");
}


namespace evb {
  namespace readoutunit {

    template<>
    void BUproxy<EVM>::fillRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest) const
    {
      fragmentRequest->ruTids = readoutUnit_->getRUtids();
    }

    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      // Only get a request if we also have data to go with it.
      // Otherwise, we consume a request at the end of the run which gets lost
      FragmentChainPtr superFragment;
      if ( fragmentRequestFIFO_.empty() || !input_->getNextAvailableSuperFragment(superFragment) ) return false;

      fragmentRequestFIFO_.deq(fragmentRequest);

      fragmentRequest->evbIds.clear();
      fragmentRequest->evbIds.reserve(fragmentRequest->nbRequests);

      superFragments.push_back(superFragment);
      EvBid evbId = superFragment->getEvBid();
      fragmentRequest->evbIds.push_back(evbId);

      const uint32_t lumiSectionOfFirstFragment = evbId.lumiSection();
      uint32_t remainingRequests = fragmentRequest->nbRequests - 1;
      const uint32_t maxTries = configuration_->maxTriggerAgeMSec*100;
      uint32_t tries = 0;
      while (
        doProcessing_ &&
        remainingRequests > 0 &&
        evbId.lumiSection() == lumiSectionOfFirstFragment &&
        tries < maxTries
      )
      {
        if ( input_->getNextAvailableSuperFragment(superFragment) )
        {
          superFragments.push_back(superFragment);
          evbId = superFragment->getEvBid();
          fragmentRequest->evbIds.push_back(evbId);
          --remainingRequests;
        }
        else
        {
          ::usleep(10);
          ++tries;
        }
      }

      fragmentRequest->nbRequests = fragmentRequest->evbIds.size();

      readoutUnit_->getRUproxy()->sendRequest(fragmentRequest);

      return true;
    }


    template<>
    bool BUproxy<EVM>::isEmpty() const
    {
      return ( processesActive_.none() && readoutUnit_->getRUproxy()->isEmpty() );
    }


    template<>
    void Configuring<EVM>::doConfigure()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->configure();
    }


    template<>
    void Running<EVM>::doStartProcessing(const uint32_t runNumber)
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->startProcessing();
    }


    template<>
    void Running<EVM>::doStopProcessing()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->stopProcessing();
    }


    template<>
    void Draining<EVM>::doDraining()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->drain();
    }


    template<>
    void evm::ReadoutUnit::addComponentsToWebPage
    (
      cgicc::table& table
    ) const
    {
      using namespace cgicc;

      table.add(tr()
        .add(td(input_->getHtmlSnipped()).set("class","xdaq-evb-component").set("rowspan","2"))
        .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
        .add(td(dynamic_cast<const EVM*>(this)->getRUproxy()->getHtmlSnipped()).set("class","xdaq-evb-component")));

      table.add(tr()
        .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
        .add(td(buProxy_->getHtmlSnipped()).set("class","xdaq-evb-component")));
    }
  }
}


/**
 * Provides the factory method for the instantiation of EVM applications.
 */
XDAQ_INSTANTIATOR_IMPL(evb::EVM)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
