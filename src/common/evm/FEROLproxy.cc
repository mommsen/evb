#include <boost/bind.hpp>

#include "interface/shared/GlobalEventNumber.h"
#include "evb/EVM.h"
#include "evb/evm/EVMinput.h"


bool evb::evm::EVMinput::FEROLproxy::getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
{
  readoutunit::FragmentChain::FragmentPtr fragment;

  if ( ! masterFED_->second->deq(fragment) ) return false;

  const EvBid& evbId = fragment->evbId;
  superFragment.reset( new readoutunit::FragmentChain(fragment) );

  for (FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    if ( it != masterFED_ )
    {
      while ( doProcessing_ && !it->second->deq(fragment) ) {};

      if ( ! doProcessing_ ) return false;

      if ( evbId != fragment->evbId )
      {
        std::ostringstream oss;
        oss << "Mismatch detected: expected evb id "
          << evbId << ", but found evb id "
          << fragment->evbId
          << " in data block from FED "
          << it->first;
        XCEPT_RAISE(exception::MismatchDetected, oss.str());
      }

      superFragment->append(fragment);
    }
  }

  toolbox::mem::Reference* bufRef = 0;
  if ( getScalerFragment(evbId,bufRef) )
    superFragment->append(bufRef);

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
      triggerFedId_ = TCDS_FED_ID;
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromTCDS, this, _1);
      return;
    }

    masterFED_ = fragmentFIFOs_.find(GTP_FED_ID);
    if ( masterFED_ != fragmentFIFOs_.end() )
    {
      triggerFedId_ = GTP_FED_ID;
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTP, this, _1);
      return;
    }

    masterFED_ = fragmentFIFOs_.find(GTPe_FED_ID);
    if ( masterFED_ != fragmentFIFOs_.end() )
    {
      triggerFedId_ = GTPe_FED_ID;
      lumiSectionFunction_ = boost::bind(&evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTPe, this, _1);
      return;
    }
  }

  // no FED to get LS from. Thus use the first FED and fake the LS
  triggerFedId_ = FED_COUNT + 1;
  masterFED_ = fragmentFIFOs_.begin();
  evbIdFactories_[masterFED_->first].setFakeLumiSectionDuration(configuration->fakeLumiSectionDuration);
}


evb::EvBid evb::evm::EVMinput::FEROLproxy::getEvBid
(
  const uint16_t fedId,
  const uint32_t eventNumber,
  const unsigned char* payload
)
{
  if ( fedId != triggerFedId_ )
  {
    return evbIdFactories_[fedId].getEvBid(eventNumber);
  }
  else
  {
    const uint32_t lsNumber = lumiSectionFunction_(payload);
    return evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
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

  //check that we've got the FDL chip
  if (! has_evm_fdl(payload) )
  {
    std::ostringstream msg;
    msg << "No FDL chip found in GTP FED " << GTP_FED_ID;
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
