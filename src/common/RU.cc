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
    void BUproxy<RU>::fillRequest(const msg::RqstForFragmentsMsg* rqstMsg, FragmentRequest& fragmentRequest)
    {
      if ( rqstMsg->nbRequests < 0 )
      {
        std::ostringstream oss;
        oss << "Got a fragment request message BU TID " << fragmentRequest.buTid;
        oss << " not specifying any EvB ids.";
        oss << " This is a non-valid request to a RU.";
        XCEPT_RAISE(exception::Configuration, oss.str());
      }  
      
      fragmentRequest.nbRequests = rqstMsg->nbRequests;
      for (int32_t i = 0; i < rqstMsg->nbRequests; ++i)
        fragmentRequest.evbIds.push_back( rqstMsg->evbIds[i] );
    }
    
    template<>
    bool BUproxy<RU>::processRequest(FragmentRequest& fragmentRequest, SuperFragments& superFragments)
    {
      for (uint32_t i=0; i < fragmentRequest.nbRequests; ++i)
      {
        const EvBid& evbId = fragmentRequest.evbIds.at(i);
        FragmentChainPtr superFragment;
        
        while ( doProcessing_ &&
          !input_->getSuperFragmentWithEvBid(evbId, superFragment) ) ::sched_yield(); //::usleep(1000);
        
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
