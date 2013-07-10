#ifndef _evb_evm_h_
#define _evb_evm_h_

#include <vector>

#include <boost/shared_ptr.hpp>

#include "evb/evm/EVMinput.h"
#include "evb/evm/RUproxy.h"
#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "i2o/i2oDdmLib.h"
#include "xdaq/ApplicationStub.h"


namespace evb {
  
  class EVM;
  
  namespace evm {
    typedef readoutunit::StateMachine<EVM> EVMStateMachine;
    typedef readoutunit::ReadoutUnit<EVM,readoutunit::Configuration,EVMStateMachine> ReadoutUnit;
  }
  
  /**
   * \ingroup xdaqApps
   * \brief Event Manager (EVM)
   */
  class EVM : public evm::ReadoutUnit
  {
  public:
    
    EVM(xdaq::ApplicationStub*);
    
    XDAQ_INSTANTIATOR();
    
    typedef boost::shared_ptr<evm::RUproxy> RUproxyPtr;
    RUproxyPtr getRUproxy() const
    { return ruProxy_; }
    
    std::vector<I2O_TID>& getRUtids() const
    { return ruProxy_->getRUtids(); }
    
    void allocateFIFOWebPage(xgi::Input*, xgi::Output*);
    
  private:

    boost::shared_ptr<evm::RUproxy> ruProxy_;
    
    
  }; // class EVM
  
} // namespace evb

#endif // _evb_evm_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
