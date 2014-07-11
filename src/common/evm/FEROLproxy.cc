#include <boost/bind.hpp>

#include "interface/shared/GlobalEventNumber.h"
#include "evb/EVM.h"
#include "evb/evm/EVMinput.h"


bool evb::evm::EVMinput::FEROLproxy::getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
{
  FedFragmentPtr fragment;

  if ( masterFED_ == fragmentFIFOs_.end() || !masterFED_->second->deq(fragment) ) return false;

  const EvBid referenceEvBid = getEvbIdForMasterFed(fragment);
  superFragment.reset( new readoutunit::FragmentChain(referenceEvBid,fragment) );

  for (FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    if ( it != masterFED_ )
    {
      while ( doProcessing_ && !it->second->deq(fragment) ) {};

      if ( ! doProcessing_ ) return false;

      const EvBid evbId = evbIdFactories_[fragment->getFedId()].getEvBid(fragment->getEventNumber());
      superFragment->append(evbId,fragment);
    }
  }

  return true;
}


void evb::evm::EVMinput::FEROLproxy::configure(boost::shared_ptr<readoutunit::Configuration> configuration)
{
  readoutunit::Input<EVM,readoutunit::Configuration>::FEROLproxy::configure(configuration);

  if ( configuration->getLumiSectionFromTrigger )
  {
    masterFED_ = fragmentFIFOs_.find(TCDS_FED_ID);
    if ( masterFED_ != fragmentFIFOs_.end() )
    {
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromTCDS, this, _1);
      return;
    }

    masterFED_ = fragmentFIFOs_.find(GTP_FED_ID);
    if ( masterFED_ != fragmentFIFOs_.end() )
    {
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTP, this, _1);
      return;
    }

    masterFED_ = fragmentFIFOs_.find(GTPe_FED_ID);
    if ( masterFED_ != fragmentFIFOs_.end() )
    {
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTPe, this, _1);
      return;
    }
  }

  // no FED to get LS from. Thus use the first FED and fake the LS
  lumiSectionFunction_ = 0;
  masterFED_ = fragmentFIFOs_.begin();
  if ( masterFED_ != fragmentFIFOs_.end() )
    evbIdFactories_[masterFED_->first].setFakeLumiSectionDuration(configuration->fakeLumiSectionDuration);
}


evb::EvBid evb::evm::EVMinput::FEROLproxy::getEvbIdForMasterFed
(
  const evb::FedFragmentPtr& fragment
)
{
  if ( lumiSectionFunction_ )
  {
    unsigned char* payload = (unsigned char*)fragment->getBufRef()
      + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + sizeof(ferolh_t);
    const uint32_t lsNumber = lumiSectionFunction_(payload);
    return evbIdFactories_[masterFED_->first].getEvBid(fragment->getEventNumber(),lsNumber);
  }
  else
  {
    return evbIdFactories_[masterFED_->first].getEvBid(fragment->getEventNumber());
  }
}


uint32_t evb::evm::EVMinput::FEROLproxy::getLumiSectionFromTCDS(const unsigned char* payload) const
{
  return 0;
}


uint32_t evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTP(const unsigned char* payload) const
{
  using namespace evtn;

  //set the evm board sense
  if (! set_evm_board_sense(payload) )
  {
    std::ostringstream msg;
    msg << "Cannot decode EVM board sense for GTP FED " << GTP_FED_ID;
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }

  //check that we've got the TCS chip
  if (! has_evm_tcs(payload) )
  {
    std::ostringstream msg;
    msg << "No TCS chip found in GTP FED " << GTP_FED_ID;
    XCEPT_RAISE(exception::L1Trigger, "No TCS chip found");
  }

  //check that we've got the FDL chip
  if (! has_evm_fdl(payload) )
  {
    std::ostringstream msg;
    msg << "No FDL chip found in GTP FED " << GTP_FED_ID;
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }

  //check that we got the right bunch crossing
  if ( getfdlbxevt(payload) != 0 )
  {
    std::ostringstream msg;
    msg << "Wrong bunch crossing in event (expect 0): "
      << getfdlbxevt(payload);
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }

  //extract lumi section number
  //use offline numbering scheme where LS starts with 1
  return getlbn(payload) + 1;
}


uint32_t evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTPe(const unsigned char* payload) const
{
  using namespace evtn;

  if (! gtpe_board_sense(payload) )
  {
    std::ostringstream msg;
    msg << "Received trigger fragment from GTPe FED " << GTPe_FED_ID << " without GTPe board id.";
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }

  const uint32_t orbitNumber = gtpe_getorbit(payload);

  return (orbitNumber / ORBITS_PER_LS) + 1;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
