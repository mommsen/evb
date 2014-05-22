#ifndef _evb_ru_RUinput_h_
#define _evb_ru_RUinput_h_

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

  class RU;

  namespace ru {

    /**
     * \ingroup xdaqApps
     * \brief Event fragment input handler of RU
     */

    class RUinput : public readoutunit::Input<RU,readoutunit::Configuration>
    {

    public:

      RUinput(RU* ru) :
      readoutunit::Input<RU,readoutunit::Configuration>(ru) {};

    private:

      class FEROLproxy : public readoutunit::Input<RU,readoutunit::Configuration>::FEROLproxy
      {
      public:

        FEROLproxy(RUinput* input)
        : readoutunit::Input<RU,readoutunit::Configuration>::FEROLproxy(input) {};

        virtual bool getSuperFragmentWithEvBid(const EvBid&, readoutunit::FragmentChainPtr&);
      };

      class DummyInputData : public readoutunit::Input<RU,readoutunit::Configuration>::DummyInputData
      {
      public:

        DummyInputData(RUinput* input)
        : readoutunit::Input<RU,readoutunit::Configuration>::DummyInputData(input) {};

        virtual bool getSuperFragmentWithEvBid(const EvBid& evbId, readoutunit::FragmentChainPtr& superFragment)
        { return createSuperFragment(evbId,superFragment); }

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


  } } //namespace evb::ru


#endif // _evb_ru_RUinput_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
