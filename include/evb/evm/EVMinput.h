#ifndef _evb_evm_EVMInput_h_
#define _evb_evm_EVMInput_h_

#include <boost/scoped_ptr.hpp>

#include <map>
#include <stdint.h>

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

    class EVMinput : public readoutunit::Input<EVM,readoutunit::Configuration>
    {

    public:

      EVMinput(EVM* evm) :
      readoutunit::Input<EVM,readoutunit::Configuration>(evm) {};

    private:

      class FEROLproxy : public readoutunit::Input<EVM,readoutunit::Configuration>::FEROLproxy
      {
      public:

        FEROLproxy(EVMinput* input)
        : readoutunit::Input<EVM,readoutunit::Configuration>::FEROLproxy(input) {};

        virtual bool getNextAvailableSuperFragment(readoutunit::FragmentChainPtr&);
        virtual void configure(boost::shared_ptr<readoutunit::Configuration>);

      private:

        virtual EvBid getEvBid(const uint16_t fedId, const uint32_t eventNumber, const unsigned char* payload);
        virtual uint32_t getLumiSectionFromTCDS(const unsigned char*) const;
        virtual uint32_t getLumiSectionFromGTP(const unsigned char*) const;
        virtual uint32_t getLumiSectionFromGTPe(const unsigned char*) const;

        FragmentFIFOs::iterator masterFED_;

      };

      class DummyInputData : public readoutunit::Input<EVM,readoutunit::Configuration>::DummyInputData
      {
      public:

        DummyInputData(EVMinput* input)
        : readoutunit::Input<EVM,readoutunit::Configuration>::DummyInputData(input) {};

        virtual bool getNextAvailableSuperFragment(readoutunit::FragmentChainPtr& superFragment)
        {
          if ( ! doProcessing_ ) return false;

          const EvBid evbId = evbIdFactory_.getEvBid();

          return createSuperFragment(evbId,superFragment);
        }

      };

      virtual void getHandlerForInputSource(boost::shared_ptr<Handler>& handler)
      {
        const std::string inputSource = configuration_->inputSource.toString();

        if ( inputSource == "FEROL" )
        {
          handler.reset( new FEROLproxy(this) );
        }
        else if ( inputSource == "Local" )
        {
          handler.reset( new DummyInputData(this) );
        }
        else
        {
          XCEPT_RAISE(exception::Configuration,
                      "Unknown input source " + inputSource + " requested");
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
