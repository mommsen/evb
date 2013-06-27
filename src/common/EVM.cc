#include "evb/EVM.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/States.h"
#include "evb/evm/EVMinput.h"


evb::EVM::EVM(xdaq::ApplicationStub* app) :
evm::ReadoutUnit(app,"/evb/images/evm64x64.gif")
{
  //this->initialize();
  
  LOG4CPLUS_INFO(this->logger_, "End of constructor");
}


namespace evb {
  namespace readoutunit {
    
    template<>
    void BUproxy<evm::ReadoutUnit>::fillRequest(const msg::RqstForFragmentsMsg* rqstMsg, FragmentRequest& request)
    {
      if ( rqstMsg->nbRequests > 0 )
      {
        std::ostringstream oss;
        oss << "Got a fragment request message BU TID " << request.buTid;
        oss << " specifying " << rqstMsg->nbRequests << " EvB ids.";
        oss << " This is a non-valid request to the EVM.";
        XCEPT_RAISE(exception::Configuration, oss.str());
      }
      
      request.nbRequests = abs(rqstMsg->nbRequests);
    }
    
    template<>
    void BUproxy<evm::ReadoutUnit>::processRequest(FragmentRequest& request)
    {
      request.evbIds.clear();
      request.evbIds.reserve(request.nbRequests);
      
      uint32_t tries = 0;
      SuperFragments superFragments;
      FragmentChainPtr superFragment;
      
      while ( doProcessing_ && !readoutUnit_->getInput()->getNextAvailableSuperFragment(superFragment) ) ::usleep(1000);
      
      if ( superFragment->isValid() )
      {
        superFragments.push_back(superFragment);
        request.evbIds.push_back( superFragment->getEvBid() );
      }
      
      while ( doProcessing_ && request.evbIds.size() < request.nbRequests && tries < configuration_->maxTriggerAgeMSec )
      {
        if ( readoutUnit_->getInput()->getNextAvailableSuperFragment(superFragment) )
        {
          superFragments.push_back(superFragment);
          request.evbIds.push_back( superFragment->getEvBid() );
        }
        else
        {
          ::usleep(1000);
        }
        ++tries;
      }
      
      request.nbRequests = request.evbIds.size();
      sendData(request, superFragments);
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
