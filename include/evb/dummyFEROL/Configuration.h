#ifndef _evb_test_dummyFEROL_configuration_h_
#define _evb_test_dummyFEROL_configuration_h_

#include <memory>

#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {
  namespace test {
    namespace dummyFEROL {

      /**
       * \ingroup xdaqApps
       * \brief Configuration for the builder unit (BU)
       */
      struct Configuration
      {
        xdata::String sourceHost;
        xdata::UnsignedInteger32 sourcePort;
        xdata::String destinationHost;
        xdata::UnsignedInteger32 destinationPort;
        xdata::UnsignedInteger32 fedId;
        xdata::UnsignedInteger32 fedSize;
        xdata::UnsignedInteger32 fedSizeStdDev;
        xdata::UnsignedInteger32 minFedSize;
        xdata::UnsignedInteger32 maxFedSize;
        xdata::Boolean computeCRC;
        xdata::Boolean usePlayback;
        xdata::String playbackDataFile;
        xdata::UnsignedInteger32 frameSize;
        xdata::UnsignedInteger32 fragmentFIFOCapacity;
        xdata::UnsignedInteger32 fakeLumiSectionDuration;
        xdata::UnsignedInteger32 maxTriggerRate;

        Configuration()
          : sourceHost("localhost"),
            sourcePort(0),
            destinationHost("localhost"),
            destinationPort(0),
            fedId(0),
            fedSize(2048),
            fedSizeStdDev(0),
            minFedSize(16), // minimum to fit FED header and trailer
            maxFedSize(0), // no limitiation
            computeCRC(true),
            usePlayback(false),
            playbackDataFile(""),
            frameSize(0x40000),
            fragmentFIFOCapacity(32),
            fakeLumiSectionDuration(23),
            maxTriggerRate(0)
        {};

        void addToInfoSpace
        (
          InfoSpaceItems& params,
          xdaq::ApplicationContext* context
        )
        {
          params.add("sourceHost", &sourceHost);
          params.add("sourcePort", &sourcePort);
          params.add("destinationHost", &destinationHost);
          params.add("destinationPort", &destinationPort);
          params.add("fedId", &fedId);
          params.add("fedSize", &fedSize);
          params.add("fedSizeStdDev", &fedSizeStdDev);
          params.add("minFedSize", &minFedSize);
          params.add("maxFedSize", &maxFedSize);
          params.add("computeCRC", &computeCRC);
          params.add("usePlayback", &usePlayback);
          params.add("playbackDataFile", &playbackDataFile);
          params.add("frameSize", &frameSize);
          params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
          params.add("fakeLumiSectionDuration", &fakeLumiSectionDuration);
          params.add("maxTriggerRate", &maxTriggerRate, InfoSpaceItems::change);
        }
      };

      using ConfigurationPtr = std::shared_ptr<Configuration>;

    } } } // namespace evb::test::dummyFEROL

#endif // _evb_test_dummyFEROL_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
