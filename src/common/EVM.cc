#include <sched.h>

#include "evb/EVM.h"
#include "evb/evm/Configuration.h"
#include "evb/readoutunit/BUproxy.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/FerolConnectionManager.h"
#include "evb/readoutunit/Input.h"
#include "evb/readoutunit/States.h"
#include "interface/shared/GlobalEventNumber.h"
#include "xgi/Method.h"


evb::EVM::EVM(xdaq::ApplicationStub* app) :
  evm::ReadoutUnit(app,"/evb/images/evm64x64.gif")
{
  this->stateMachine_.reset( new evm::EVMStateMachine(this) );
  this->input_.reset( new readoutunit::Input<EVM,evm::Configuration>(this) );
  this->ferolConnectionManager_.reset( new readoutunit::FerolConnectionManager<EVM,evm::Configuration>(this) );
  this->ruProxy_.reset( new evm::RUproxy(this,this->stateMachine_) );
  this->buProxy_.reset( new readoutunit::BUproxy<EVM>(this) );

  this->initialize();

  LOG4CPLUS_INFO(this->getApplicationLogger(), "End of constructor");
}


void evb::EVM::do_appendMonitoringInfoSpaceItems(InfoSpaceItems& items)
{
  evm::ReadoutUnit::do_appendMonitoringInfoSpaceItems(items);
  ruProxy_->appendMonitoringItems(items);
}


void evb::EVM::do_updateMonitoringInfo()
{
  evm::ReadoutUnit::do_updateMonitoringInfo();
  ruProxy_->updateMonitoringItems();
}


void evb::EVM::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "maxTriggerRate")
  {
    const uint32_t triggerRate = this->configuration_->maxTriggerRate;
    std::ostringstream msg;
    msg << "Setting maxTriggerRate to " << triggerRate << " Hz";
    LOG4CPLUS_INFO(this->getApplicationLogger(),msg.str());
    input_->setMaxTriggerRate(triggerRate);
  }
  else
  {
    evm::ReadoutUnit::do_handleItemChangedEvent(item);
  }
}


namespace evb {
  namespace readoutunit {

    template<>
    uint32_t evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromTCDS(const DataLocations& dataLocations) const
    {
      using namespace evtn;

      uint32_t offset = sizeof(fedh_t) + 7 * SLINK_WORD_SIZE + SLINK_HALFWORD_SIZE;
      DataLocations::const_iterator it = dataLocations.begin();
      const DataLocations::const_iterator itEnd = dataLocations.end();

      while ( it != itEnd && offset > it->iov_len )
      {
        offset -= it->iov_len;
        ++it;
      }

      if ( it == itEnd )
      {
        std::ostringstream msg;
        msg << "Premature end of TCDS data block from FED " << TCDS_FED_ID;
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      return *(uint32_t*)((unsigned char*)it->iov_base + offset) & 0xffffffff;
    }


    template<>
    uint32_t evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTPe(const DataLocations& dataLocations) const
    {
      using namespace evtn;

      uint32_t offset = GTPE_ORBTNR_OFFSET * SLINK_HALFWORD_SIZE; // includes FED header

      DataLocations::const_iterator it = dataLocations.begin();
      const DataLocations::const_iterator itEnd = dataLocations.end();

      while ( it != itEnd && offset > it->iov_len )
      {
        offset -= it->iov_len;
        ++it;
      }

      if ( it == itEnd )
      {
        std::ostringstream msg;
        msg << "Premature end of GTPe data block from FED " << GTPe_FED_ID;
        XCEPT_RAISE(exception::TCDS, msg.str());
      }

      const uint32_t orbitNumber = *(uint32_t*)((unsigned char*)it->iov_base + offset);

      return (orbitNumber / ORBITS_PER_LS) + 1;
    }


    template<>
    void evb::readoutunit::Input<EVM,evm::Configuration>::setMasterStream()
    {
      masterStream_ = ferolStreams_.end();

      if ( ferolStreams_.empty() || readoutUnit_->getConfiguration()->dropInputData ) return;

      EvBidFactory::LumiSectionFunction lumiSectionFunction = 0;

      if ( readoutUnit_->getConfiguration()->getLumiSectionFromTrigger )
      {
        masterStream_ = ferolStreams_.find(TCDS_FED_ID);
        if ( masterStream_ != ferolStreams_.end() )
        {
          lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromTCDS, this, _1);
          LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), "Using TCDS as lumi section source");
        }
        else
        {
          masterStream_ = ferolStreams_.find(GTPe_FED_ID);
          if ( masterStream_ != ferolStreams_.end() )
          {
            lumiSectionFunction = boost::bind(&evb::readoutunit::Input<EVM,evm::Configuration>::getLumiSectionFromGTPe, this, _1);
            LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(), "Using GTPe as lumi section source");
          }
        }
      }

      if ( masterStream_ == ferolStreams_.end() )
        masterStream_ = ferolStreams_.begin();

      masterStream_->second->useAsMaster();

      const EvBidFactoryPtr evbIdFactory = masterStream_->second->getEvBidFactory();
      if ( lumiSectionFunction )
      {
        evbIdFactory->setLumiSectionFunction(lumiSectionFunction);
      }
      else
      {
        const uint32_t lsDuration = readoutUnit_->getConfiguration()->fakeLumiSectionDuration;
        evbIdFactory->setFakeLumiSectionDuration(lsDuration);

        std::ostringstream msg;
        msg << "Emulating a lumi section duration of " << lsDuration << "s";
        LOG4CPLUS_INFO(readoutUnit_->getApplicationLogger(),msg.str());
      }
    }


    template<>
    void BUproxy<EVM>::handleRequest(const msg::EventRequest* eventRequest, FragmentRequestPtr& fragmentRequest)
    {
      fragmentRequest->nbDiscards = eventRequest->nbRequests; //Always keep nb discards == nb requests for RUs
      fragmentRequest->ruTids = readoutUnit_->getRUtids();

      boost::upgrade_lock<boost::shared_mutex> sl(fragmentRequestFIFOsMutex_);

      FragmentRequestFIFOs::iterator pos = fragmentRequestFIFOs_.lower_bound(eventRequest->buTid);
      if ( pos == fragmentRequestFIFOs_.end() || fragmentRequestFIFOs_.key_comp()(eventRequest->buTid,pos->first) )
      {
        // new TID
        boost::upgrade_to_unique_lock< boost::shared_mutex > ul(sl);
        PrioritizedFragmentRequestFIFOs prioritizedFragmentRequestFIFOs;
        prioritizedFragmentRequestFIFOs.reserve(evb::LOWEST_PRIORITY+1);
        for ( uint16_t priority = 0; priority <= evb::LOWEST_PRIORITY; ++priority)
        {
          std::ostringstream name;
          name << "fragmentRequestFIFO_BU" << eventRequest->buTid << "_priority" << priority;
          const FragmentRequestFIFOPtr requestFIFO( new FragmentRequestFIFO(readoutUnit_,name.str()) );
          requestFIFO->resize(readoutUnit_->getConfiguration()->fragmentRequestFIFOCapacity);
          prioritizedFragmentRequestFIFOs.push_back(requestFIFO);
        }
        pos = fragmentRequestFIFOs_.insert(pos, FragmentRequestFIFOs::value_type(
                                             eventRequest->buTid,prioritizedFragmentRequestFIFOs));
        nextBU_ = pos; //make sure the nextBU points to a valid location
      }

      pos->second[eventRequest->priority]->enqWait(fragmentRequest);
    }


    template<>
    void BUproxy<EVM>::doLumiSectionTransition()
    {
      // Assure that each BU gets a request served when the lumi section changes.
      // Start with the lowest priority queue to eventually
      // unblock any old requests sent with lower priortity.

      boost::unique_lock<boost::shared_mutex> ul(fragmentRequestFIFOsMutex_);

      for ( FragmentRequestFIFOs::iterator it = fragmentRequestFIFOs_.begin(), itEnd = fragmentRequestFIFOs_.end();
            it != itEnd; ++it )
      {
        FragmentRequestPtr fragmentRequest;
        for ( uint16_t p = evb::LOWEST_PRIORITY; p > 0; --p )
        {
          if ( it->second[p]->deq(fragmentRequest) )
          {
            it->second[0]->enqWait(fragmentRequest);
            break;
          }
        }
      }
    }


    template<>
    bool BUproxy<EVM>::processRequest(FragmentRequestPtr& fragmentRequest, SuperFragments& superFragments)
    {
      boost::mutex::scoped_lock prm(processingRequestMutex_);

      SuperFragmentPtr superFragment;

      try
      {
        {
          boost::shared_lock<boost::shared_mutex> frm(fragmentRequestFIFOsMutex_);

          if ( ! doProcessing_ ) return false;

          if ( fragmentRequestFIFOs_.empty() ) return false;

          // search the next request FIFO with highest priority (lowest number) which is non-empty
          const FragmentRequestFIFOs::iterator lastBU = nextBU_;
          uint16_t priority = 0;
          while ( priority <= evb::LOWEST_PRIORITY &&
                  nextBU_->second[priority]->empty() )
          {
            if ( ++nextBU_ == fragmentRequestFIFOs_.end() )
              nextBU_ = fragmentRequestFIFOs_.begin();

            if ( lastBU == nextBU_ ) ++priority;
          }

          if ( priority > evb::LOWEST_PRIORITY ) return false;

          if ( !input_->getNextAvailableSuperFragment(superFragment) ) return false;

          // this must succeed as we checked that the queue is non-empty
          assert( nextBU_->second[priority]->deq(fragmentRequest) );
          if ( ++nextBU_ == fragmentRequestFIFOs_.end() )
            nextBU_ = fragmentRequestFIFOs_.begin();
        }

        fragmentRequest->evbIds.clear();
        fragmentRequest->evbIds.reserve(fragmentRequest->nbRequests);

        superFragments.push_back(superFragment);
        fragmentRequest->evbIds.push_back( superFragment->getEvBid() );

        uint32_t remainingRequests = fragmentRequest->nbRequests - 1;
        const uint32_t maxTries = readoutUnit_->getConfiguration()->maxTriggerAgeMSec*10;
        uint32_t tries = 0;
        while ( remainingRequests > 0 && tries < maxTries )
        {
          if ( input_->getNextAvailableSuperFragment(superFragment) )
          {
            superFragments.push_back(superFragment);
            fragmentRequest->evbIds.push_back( superFragment->getEvBid() );
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
