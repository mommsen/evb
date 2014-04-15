#include "interface/shared/GlobalEventNumber.h"
#include "evb/EVM.h"
#include "evb/evm/EVMinput.h"


bool evb::evm::EVMinput::FEROLproxy::getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
{
  // if we got a superFragment from pt::frl, use it
  if ( superFragmentFIFO_.deq(superFragment) ) return true;

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

  return true;
}


void evb::evm::EVMinput::FEROLproxy::configure(boost::shared_ptr<readoutunit::Configuration> configuration)
{
  readoutunit::Input<EVM,readoutunit::Configuration>::FEROLproxy::configure(configuration);

  masterFED_ = fragmentFIFOs_.find(GTP_FED_ID);
  if ( masterFED_ != fragmentFIFOs_.end() ) return;

  masterFED_ = fragmentFIFOs_.find(GTPe_FED_ID);
  if ( masterFED_ != fragmentFIFOs_.end() ) return;

  // no trigger FED, use the first FED
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
  switch(fedId)
  {
    case GTP_FED_ID:
    {
      const uint32_t lsNumber = getLumiSectionFromGTP(payload);
      return evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
    }
    case GTPe_FED_ID:
    {
      const uint32_t lsNumber = getLumiSectionFromGTPe(payload);
      return evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
    }
    default:
      return evbIdFactories_[fedId].getEvBid(eventNumber);
  }
}


uint32_t evb::evm::EVMinput::FEROLproxy::getLumiSectionFromGTP(const unsigned char* payload) const
{
  using namespace evtn;

  //set the evm board sense
  if (! set_evm_board_sense(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "Cannot decode EVM board sense");
  }

  //check that we've got the TCS chip
  if (! has_evm_tcs(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "No TCS chip found");
  }

  //check that we've got the FDL chip
  if (! has_evm_fdl(payload) )
  {
    XCEPT_RAISE(exception::L1Trigger, "No FDL chip found");
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
    XCEPT_RAISE(exception::L1Trigger, "Received trigger fragment without GPTe board id.");
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
