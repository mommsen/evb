#ifndef _evb_evm_h_
#define _evb_evm_h_

#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "xdaq/ApplicationStub.h"


namespace evb {

  namespace evm {
    class EVMinput;
    typedef readoutunit::ReadoutUnit<readoutunit::Configuration,EVMinput> ReadoutUnit;
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
    
    
  private:
    
  }; // class EVM
  
} // namespace evb

#endif // _evb_evm_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
