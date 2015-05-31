#include <sched.h>

#include "evb/EVM.h"
#include "evb/evm/Configuration.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/States.h"
#include "interface/shared/GlobalEventNumber.h"
#include "xgi/Method.h"


evb::EVM::EVM(xdaq::ApplicationStub* app) :
  evm::ReadoutUnit(app,"/evb/images/evm64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  this->stateMachine_.reset( new evm::EVMStateMachine(this) );
  this->input_.reset( new evm::Input(this) );
  this->ruProxy_.reset( new evm::RUproxy(this,this->stateMachine_,fastCtrlMsgPool) );
  this->buProxy_.reset( new readoutunit::BUproxy<EVM>(this) );

  this->initialize();

  LOG4CPLUS_INFO(this->getApplicationLogger(), "End of constructor");
}


namespace evb {
  namespace readoutunit {

    template<>
    uint32_t evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromTCDS(const unsigned char* payload) const
    {
      using namespace evtn;

      return *(uint32_t*)(payload + sizeof(fedh_t) +
                          7 * SLINK_WORD_SIZE + SLINK_HALFWORD_SIZE) & 0xffffffff;
    }


    template<>
    uint32_t evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTP(const unsigned char* payload) const
    {
      using namespace evtn;

      //set the evm board sense
      if (! set_evm_board_sense(payload) )
      {
        std::ostringstream msg;
        msg << "Cannot decode EVM board sense for GTP FED " << GTP_FED_ID;
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      //check that we've got the FDL chip
      if (! has_evm_fdl(payload) )
      {
        std::ostringstream msg;
        msg << "No FDL chip found in GTP FED " << GTP_FED_ID;
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      //check that we got the right bunch crossing
      if ( getfdlbxevt(payload) != 0 )
      {
        std::ostringstream msg;
        msg << "Wrong bunch crossing in event (expect 0): "
          << getfdlbxevt(payload);
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      //extract lumi section number
      const uint32_t ls = (*(uint32_t*)( payload + sizeof(fedh_t) +
                                         ( EVM_GTFE_BLOCK + EVM_TCS_BLOCK
                                           + EVM_FDL_BLOCK * (EVM_FDL_NOBX/2) ) * SLINK_WORD_SIZE +
                                         12 * SLINK_HALFWORD_SIZE) & 0xffff0000 ) >> 16;

      //use offline numbering scheme where LS starts with 1
      return ls + 1;
    }


    template<>
    uint32_t evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTPe(const unsigned char* payload) const
    {
      using namespace evtn;

      if (! gtpe_board_sense(payload) )
      {
        std::ostringstream msg;
        msg << "Received trigger fragment from GTPe FED " << GTPe_FED_ID << " without GTPe board id.";
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      const uint32_t orbitNumber = gtpe_getorbit(payload);

      return (orbitNumber / ORBITS_PER_LS) + 1;
    }


    template<>
    void evb::readoutunit::Input<EVM,evm::Configuration>::setMasterStream()
    {
      masterStream_ = ferolStreams_.end();

      if ( ferolStreams_.empty() || readoutUnit_->getConfiguration()->dropInputData ) return;

      boost::function< uint32_t(const unsigned char*) > lumiSectionFunction = 0;

      if ( readoutUnit_->getConfiguration()->getLumiSectionFromTrigger )
      {
        masterStream_ = ferolStreams_.find(TCDS_FED_ID);
        if ( masterStream_ != ferolStreams_.end() )
        {
          lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromTCDS, this, _1);
        }
        else
        {
          masterStream_ = ferolStreams_.find(GTP_FED_ID);
          if ( masterStream_ != ferolStreams_.end() )
          {
            lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTP, this, _1);
          }
          else
          {
            masterStream_ = ferolStreams_.find(GTPe_FED_ID);
            if ( masterStream_ != ferolStreams_.end() )
            {
              lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTPe, this, _1);
            }
          }
        }
      }

      if ( masterStream_ == ferolStreams_.end() )
        masterStream_ = ferolStreams_.begin();

      masterStream_->second->setLumiSectionFunction(lumiSectionFunction);
    }


    template<>
    evb::EvBid evb::readoutunit::FerolStream<EVM,evm::Configuration>::getEvBid(const FedFragmentPtr& fedFragment)
    {
      const uint32_t eventNumber = fedFragment->getEventNumber();
      if ( lumiSectionFunction_ )
      {
        const uint32_t lsNumber = lumiSectionFunction_(fedFragment->getFedPayload());
        return evbIdFactory_.getEvBid(eventNumber,lsNumber);
      }
      else
      {
        return evbIdFactory_.getEvBid(eventNumber);
      }
    }


    template<>
    void BUproxy<EVM>::handleRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest)
    {
      fragmentRequest->nbDiscards = readoutMsg->nbRequests; //Always keep nb discards == nb requests for RUs
      fragmentRequest->ruTids = readoutUnit_->getRUtids();

      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      FragmentRequestFIFOs::iterator pos = fragmentRequestFIFOs_.lower_bound(readoutMsg->buTid);
      if ( pos == fragmentRequestFIFOs_.end() || fragmentRequestFIFOs_.key_comp()(readoutMsg->buTid,pos->first) )
      {
        // new TID
        std::ostringstream name;
        name << "fragmentRequestFIFO_BU" << readoutMsg->buTid;
        const FragmentRequestFIFOPtr requestFIFO( new FragmentRequestFIFO(readoutUnit_,name.str()) );
        requestFIFO->resize(readoutUnit_->getConfiguration()->fragmentRequestFIFOCapacity);
        pos = fragmentRequestFIFOs_.insert(pos, FragmentRequestFIFOs::value_type(readoutMsg->buTid,requestFIFO));
        nextBU_ = pos; //make sure the nextBU points to a valid location
      }

      pos->second->enqWait(fragmentRequest);
    }


    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      if ( fragmentRequestFIFOs_.empty() ) return false;

      // search the next request FIFO which is non-empty
      const FragmentRequestFIFOs::iterator lastBU = nextBU_;
      while ( nextBU_->second->empty() )
      {
        if ( ++nextBU_ == fragmentRequestFIFOs_.end() )
          nextBU_ = fragmentRequestFIFOs_.begin();

        if ( lastBU == nextBU_ ) return false;
      }

      try
      {
        SuperFragmentPtr superFragment;
        if ( !input_->getNextAvailableSuperFragment(superFragment) ) return false;

        processingRequest_ = true;

        // this must succeed as we checked that the queue is non-empty
        assert( nextBU_->second->deq(fragmentRequest) );
        if ( ++nextBU_ == fragmentRequestFIFOs_.end() )
          nextBU_ = fragmentRequestFIFOs_.begin();

        fragmentRequest->evbIds.clear();
        fragmentRequest->evbIds.reserve(fragmentRequest->nbRequests);

        superFragments.push_back(superFragment);
        EvBid evbId = superFragment->getEvBid();
        fragmentRequest->evbIds.push_back(evbId);

        uint32_t remainingRequests = fragmentRequest->nbRequests - 1;
        const uint32_t maxTries = readoutUnit_->getConfiguration()->maxTriggerAgeMSec*10;
        uint32_t tries = 0;
        while ( remainingRequests > 0 && tries < maxTries )
        {
          if ( input_->getNextAvailableSuperFragment(superFragment) )
          {
            superFragments.push_back(superFragment);
            evbId = superFragment->getEvBid();
            fragmentRequest->evbIds.push_back(evbId);
            --remainingRequests;
          }
          else
          {
            ::usleep(100);
            ++tries;
          }
        }
      }
      catch(exception::HaltRequested)
      {
        processingRequest_ = false;
        return false;
      }

      fragmentRequest->nbRequests = fragmentRequest->evbIds.size();

      readoutUnit_->getRUproxy()->sendRequest(fragmentRequest);

      processingRequest_ = false;
      return true;
    }


    template<>
    bool BUproxy<EVM>::isEmpty()
    {
      {
        boost::mutex::scoped_lock sl(dataMonitoringMutex_);
        if ( dataMonitoring_.outstandingEvents != 0 ) return false;
      }
      {
        boost::mutex::scoped_lock sl(processesActiveMutex_);
        if ( processesActive_.any() ) return false;
      }
      return readoutUnit_->getRUproxy()->isEmpty();
    }


    template<>
    std::string BUproxy<EVM>::getHelpTextForBuRequests() const
    {
      return "Event requests from the BUs. If no events are requested, the BU appliances are busy, have no FUs to process data or have failed.";
    }


    template<>
    void Configuring<EVM>::doConfigure(const EVM* evm)
    {
      evm->getRUproxy()->configure();
      evm->getInput()->setMaxTriggerRate(evm->getConfiguration()->maxTriggerRate);
    }


    template<>
    void Running<EVM>::doStartProcessing(const EVM* evm, const uint32_t runNumber)
    {
      evm->getRUproxy()->startProcessing();
    }


    template<>
    void Running<EVM>::doStopProcessing(const EVM* evm)
    {
      evm->getRUproxy()->stopProcessing();
    }


    template<>
    void Draining<EVM>::doDraining(const EVM* evm)
    {
      evm->getRUproxy()->drain();
    }


    template<>
    void evm::ReadoutUnit::addComponentsToWebPage
    (
      cgicc::table& table
    ) const
    {
      using namespace cgicc;

      table.add(tr()
                .add(td(input_->getHtmlSnipped()).set("class","xdaq-evb-component").set("rowspan","2"))
                .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
                .add(td(dynamic_cast<const EVM*>(this)->getRUproxy()->getHtmlSnipped()).set("class","xdaq-evb-component")));

      table.add(tr()
                .add(td(img().set("src","/evb/images/arrow_e.gif").set("alt","")))
                .add(td(buProxy_->getHtmlSnipped()).set("class","xdaq-evb-component")));
    }


    template<>
    void evm::ReadoutUnit::localItemChangedEvent(const std::string& item)
    {
      if (item == "maxTriggerRate")
      {
        const uint32_t triggerRate = this->configuration_->maxTriggerRate;
        std::ostringstream msg;
        msg << "Setting maxTriggerRate to " << triggerRate << " Hz";
        LOG4CPLUS_INFO(this->getApplicationLogger(),msg.str());
        input_->setMaxTriggerRate(triggerRate);
      }
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
