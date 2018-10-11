#ifndef _evb_evm_h_
#define _evb_evm_h_

#include <memory>
#include <vector>

#include "evb/evm/Configuration.h"
#include "evb/evm/RUproxy.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/StateMachine.h"
#include "i2o/i2oDdmLib.h"
#include "xdaq/ApplicationStub.h"


namespace evb {

  class EVM;

  namespace evm {
    using EVMStateMachine = readoutunit::StateMachine<EVM>;
    using ReadoutUnit = readoutunit::ReadoutUnit<EVM,evm::Configuration,EVMStateMachine>;
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

    using RUproxyPtr = std::shared_ptr<evm::RUproxy>;
    RUproxyPtr getRUproxy() const
    { return ruProxy_; }

    std::vector<I2O_TID>& getRUtids() const
    { return ruProxy_->getRUtids(); }

  private:

    virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
    virtual void do_updateMonitoringInfo();
    virtual void do_handleItemChangedEvent(const std::string& item);

    std::shared_ptr<evm::RUproxy> ruProxy_;


  }; // class EVM

} // namespace evb

#endif // _evb_evm_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
