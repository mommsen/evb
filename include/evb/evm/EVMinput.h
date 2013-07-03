#ifndef _evb_evm_EVMInput_h_
#define _evb_evm_EVMInput_h_

#include <boost/scoped_ptr.hpp>

#include <map>
#include <stdint.h>

#include "log4cplus/logger.h"

#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/FragmentChain.h"
#include "evb/FragmentTracker.h"
#include "evb/InfoSpaceItems.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/Input.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {

  class EVM;
  
  namespace evm {
    
    /**
     * \ingroup xdaqApps
     * \brief Event fragment input handler of EVM
     */
    
    class EVMinput : public readoutunit::Input<readoutunit::Configuration>
    {

    public:
      
      EVMinput
      (
        xdaq::ApplicationStub* app,
        boost::shared_ptr<readoutunit::Configuration> configuration
      ) :
      readoutunit::Input<readoutunit::Configuration>(app,configuration) {};
      
    private:
      
      class FEROLproxy : public readoutunit::Input<readoutunit::Configuration>::FEROLproxy
      {
      public:
        
        virtual bool getNextAvailableSuperFragment(readoutunit::FragmentChainPtr&);
        
      private:
        
        virtual uint32_t extractTriggerInformation(const unsigned char*) const;
        
      };
      
      class DummyInputData : public readoutunit::Input<readoutunit::Configuration>::DummyInputData
      {
      public:
        
        DummyInputData(EVMinput* input)
        : readoutunit::Input<readoutunit::Configuration>::DummyInputData(input) {};
        
        virtual bool getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
        {
          if (++eventNumber_ % (1 << 24) == 0) eventNumber_ = 1;
          const EvBid evbId = evbIdFactory_.getEvBid(eventNumber_);
          
          return createSuperFragment(evbId,superFragment);
        }
        
      };
      
      virtual void getHandlerForInputSource(boost::shared_ptr<Handler>& handler)
      {
        const std::string inputSource = configuration_->inputSource.toString();
        
        if ( inputSource == "FEROL" )
        {
          handler.reset( new FEROLproxy() );
        }
        else if ( inputSource == "Local" )
        {
          handler.reset( new DummyInputData(this) );
        }
        else
        {
          XCEPT_RAISE(exception::Configuration,
            "Unknown input source " + inputSource + " requested.");
        }
      }
      
    };
    
    
  } } //namespace evb::evm


#endif // _evb_evm_EVMInput_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
