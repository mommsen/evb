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
  this->input_.reset( new evm::EVMinput(app,this->configuration_) );
  this->ruProxy_.reset( new evm::RUproxy(this,fastCtrlMsgPool) );
  this->buProxy_.reset( new readoutunit::BUproxy<EVM>(this) );
  
  this->initialize();
  
  LOG4CPLUS_INFO(this->logger_, "End of constructor");
}


void evb::EVM::allocateFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "allocateFIFO");
  
  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  ruProxy_->printAllocateFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


namespace evb {
  namespace readoutunit {
    
    template<>
    void BUproxy<EVM>::fillRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest)
    {
      fragmentRequest->ruTids = readoutUnit_->getRUtids();
    }
    
    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      if ( ! fragmentRequestFIFO_.deq(fragmentRequest) ) return false;
      
      fragmentRequest->evbIds.clear();
      fragmentRequest->evbIds.reserve(fragmentRequest->nbRequests);
      
      FragmentChainPtr superFragment;
      uint32_t tries = 0;
      
      while ( doProcessing_ && !input_->getNextAvailableSuperFragment(superFragment) ) ::usleep(10);
      
      if ( superFragment->isValid() )
      {
        superFragments.push_back(superFragment);
        fragmentRequest->evbIds.push_back( superFragment->getEvBid() );
      }
      
      while ( doProcessing_ && fragmentRequest->evbIds.size() < fragmentRequest->nbRequests && tries < configuration_->maxTriggerAgeMSec*100 )
      {
        if ( input_->getNextAvailableSuperFragment(superFragment) )
        {
          superFragments.push_back(superFragment);
          fragmentRequest->evbIds.push_back( superFragment->getEvBid() );
        }
        else
        {
          ::usleep(10);
        }
        ++tries;
      }
      
      fragmentRequest->nbRequests = fragmentRequest->evbIds.size();
      
      readoutUnit_->getRUproxy()->sendRequest(fragmentRequest);
      
      return true;
    }
    
    
    template<>
    void Configuring<EVM>::doConfigure()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->configure();
    }
    
    template<>
    void Processing<EVM>::doResetMonitoringCounters()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->resetMonitoringCounters();
    }
    
    template<>
    void Clearing<EVM>::doClearing()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->clear();
    }
    
    template<>
    void Enabled<EVM>::doStartProcessing(const uint32_t runNumber)
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->startProcessing();
    }
    
    template<>
    void Enabled<EVM>::doStopProcessing()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->stopProcessing();
    }
    
    
    // template<>
    // void evm::ReadoutUnit::bindNonDefaultXgiCallbacks()
    // {
    //   ReadoutUnit::bindNonDefaultXgiCallbacks();
      
    //   // xgi::bind(
    //   //   this,
    //   //   &evb::EVM::allocateFIFOWebPage,
    //   //   "allocateFIFO"
    //   // );
    // }

   
    template<>
    void evm::ReadoutUnit::do_defaultWebPage
    (
      xgi::Output *out
    )
    {
      *out << "<tr>"                                                << std::endl;
      *out << "<td class=\"component\">"                            << std::endl;
      input_->printHtml(out);
      *out << "</td>"                                               << std::endl;
      *out << "<td>"                                                << std::endl;
      *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
      *out << "</td>"                                               << std::endl;
      *out << "<td class=\"component\">"                            << std::endl;
      buProxy_->printHtml(out);
      *out << "</td>"                                               << std::endl;
      *out << "<td>"                                                << std::endl;
      *out << "<img src=\"/evb/images/arrow_e.gif\" alt=\"\"/>"     << std::endl;
      *out << "</td>"                                               << std::endl;
      *out << "<td class=\"component\">"                            << std::endl;
      dynamic_cast<EVM*>(this)->getRUproxy()->printHtml(out);
      *out << "</td>"                                               << std::endl;
      *out << "</tr>"                                               << std::endl;
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
