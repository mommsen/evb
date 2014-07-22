#include <sched.h>

#include "evb/EVM.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/Configuration.h"
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
    uint32_t evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromTCDS(const unsigned char* payload) const
    {
      return 0;
    }


    template<>
    uint32_t evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromGTP(const unsigned char* payload) const
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

      //extract lumi section number
      //use offline numbering scheme where LS starts with 1
      return getlbn(payload) + 1;
    }


    template<>
    uint32_t evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromGTPe(const unsigned char* payload) const
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
    void evb::readoutunit::Input<EVM,readoutunit::Configuration>::setMasterStream()
    {
      masterStream_ = ferolStreams_.end();

      if ( ferolStreams_.empty() ) return;

      boost::function< uint32_t(const unsigned char*) > lumiSectionFunction = 0;

      if ( readoutUnit_->getConfiguration()->getLumiSectionFromTrigger )
      {
        masterStream_ = ferolStreams_.find(TCDS_FED_ID);
        if ( masterStream_ != ferolStreams_.end() )
        {
          lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromTCDS, this, _1);
        }
        else
        {
          masterStream_ = ferolStreams_.find(GTP_FED_ID);
          if ( masterStream_ != ferolStreams_.end() )
          {
            lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromGTP, this, _1);
          }
          else
          {
            masterStream_ = ferolStreams_.find(GTPe_FED_ID);
            if ( masterStream_ != ferolStreams_.end() )
            {
              lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,readoutunit::Configuration>::getLumiSectionFromGTPe, this, _1);
            }
          }
        }
      }

      if ( masterStream_ == ferolStreams_.end() )
        masterStream_ = ferolStreams_.begin();

      masterStream_->second->setLumiSectionFunction(lumiSectionFunction);
    }


    template<>
    evb::EvBid evb::readoutunit::FerolStream<EVM,readoutunit::Configuration>::getEvBid(const FedFragmentPtr& fedFragment)
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
    void BUproxy<EVM>::fillRequest(const msg::ReadoutMsg* readoutMsg, FragmentRequestPtr& fragmentRequest) const
    {
      fragmentRequest->nbDiscards = readoutMsg->nbRequests; //Always keep nb discards == nb requests for RUs
      fragmentRequest->ruTids = readoutUnit_->getRUtids();
    }

    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock sl(fragmentRequestFIFOmutex_);

      try
      {
        // Only get a request if we also have data to go with it.
        // Otherwise, we consume a request at the end of the run which gets lost
        FragmentChainPtr superFragment;
        if ( fragmentRequestFIFO_.empty() || !input_->getNextAvailableSuperFragment(superFragment) ) return false;

        fragmentRequestFIFO_.deq(fragmentRequest);

        fragmentRequest->evbIds.clear();
        fragmentRequest->evbIds.reserve(fragmentRequest->nbRequests);

        superFragments.push_back(superFragment);
        EvBid evbId = superFragment->getEvBid();
        fragmentRequest->evbIds.push_back(evbId);

        uint32_t remainingRequests = fragmentRequest->nbRequests - 1;
        const uint32_t maxTries = configuration_->maxTriggerAgeMSec*100;
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
            ::usleep(10);
            ++tries;
          }
        }
      }
      catch(exception::HaltRequested)
      {
        return false;
      }

      fragmentRequest->nbRequests = fragmentRequest->evbIds.size();

      readoutUnit_->getRUproxy()->sendRequest(fragmentRequest);

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
    void Configuring<EVM>::doConfigure()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->configure();
    }


    template<>
    void Running<EVM>::doStartProcessing(const uint32_t runNumber)
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->startProcessing();
    }


    template<>
    void Running<EVM>::doStopProcessing()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->stopProcessing();
    }


    template<>
    void Draining<EVM>::doDraining()
    {
      my_state::outermost_context_type& stateMachine = this->outermost_context();
      stateMachine.getOwner()->getRUproxy()->drain();
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
