#include <sched.h>

#include "evb/EVM.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/States.h"
#include "evb/evm/EVMinput.h"


evb::EVM::EVM(xdaq::ApplicationStub* app) :
evm::ReadoutUnit(app,"/evb/images/evm64x64.gif")
{
  this->stateMachine_.reset( new evm::EVMStateMachine(this) );
  this->input_.reset( new evm::EVMinput(app,this->configuration_) );
  this->buProxy_.reset( new readoutunit::BUproxy<EVM>(this) );
  
  this->initialize();
  
  LOG4CPLUS_INFO(this->logger_, "End of constructor");
}


namespace evb {
  namespace readoutunit {
    
    template<>
    void BUproxy<EVM>::fillRequest(const msg::RqstForFragmentsMsg* rqstMsg, FragmentRequest& fragmentRequest)
    {
      if ( rqstMsg->nbRequests > 0 )
      {
        std::ostringstream oss;
        oss << "Got a fragment request message BU TID " << fragmentRequest.buTid;
        oss << " specifying " << rqstMsg->nbRequests << " EvB ids.";
        oss << " This is a non-valid request to the EVM.";
        XCEPT_RAISE(exception::Configuration, oss.str());
      }
      
      fragmentRequest.nbRequests = abs(rqstMsg->nbRequests);
    }
    
    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequest& fragmentRequest, SuperFragments& superFragments)
    {

      {
        boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);
        if ( ! fragmentRequestFIFO_.deq(fragmentRequest) ) return false;
      }
      
      fragmentRequest.evbIds.clear();
      fragmentRequest.evbIds.reserve(fragmentRequest.nbRequests);
      
      FragmentChainPtr superFragment;
      uint32_t tries = 0;
      
      while ( doProcessing_ && !input_->getNextAvailableSuperFragment(superFragment) ) ::usleep(10);
      
      if ( superFragment->isValid() )
      {
        superFragments.push_back(superFragment);
        fragmentRequest.evbIds.push_back( superFragment->getEvBid() );
      }
      
      while ( doProcessing_ && fragmentRequest.evbIds.size() < fragmentRequest.nbRequests && tries < configuration_->maxTriggerAgeMSec*100 )
      {
        if ( input_->getNextAvailableSuperFragment(superFragment) )
        {
          superFragments.push_back(superFragment);
          fragmentRequest.evbIds.push_back( superFragment->getEvBid() );
        }
        else
        {
          ::usleep(10);
        }
        ++tries;
      }
      
      fragmentRequest.nbRequests = fragmentRequest.evbIds.size();

      return true;
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
