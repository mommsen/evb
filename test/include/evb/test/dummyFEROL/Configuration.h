#ifndef _evb_test_dummyFEROL_configuration_h_
#define _evb_test_dummyFEROL_configuration_h_

#include <boost/shared_ptr.hpp>

#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/Integer32.h"
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
        xdata::String destinationClass;
        xdata::UnsignedInteger32 destinationInstance;
        xdata::UnsignedInteger32 fedId;
        xdata::UnsignedInteger32 fedSize;
        xdata::UnsignedInteger32 fedSizeStdDev;
        xdata::UnsignedInteger32 minFedSize;
        xdata::UnsignedInteger32 maxFedSize;
        xdata::Boolean usePlayback;
        xdata::String playbackDataFile;
        xdata::UnsignedInteger32 frameSize;
        xdata::UnsignedInteger32 fragmentFIFOCapacity;

        Configuration()
        : destinationClass("evb::RU"),
          fedSize(2048),
          fedSizeStdDev(0),
          minFedSize(8), // minimum is 8 Bytes
          maxFedSize(0), // no limitiation
          usePlayback(false),
          playbackDataFile(""),
          frameSize(0x40000),
          fragmentFIFOCapacity(32)
        {};

        void addToInfoSpace(InfoSpaceItems& params, const uint32_t instance)
        {
          destinationInstance = instance;
          fedId = instance;

          params.add("destinationClass", &destinationClass);
          params.add("destinationInstance", &destinationInstance);
          params.add("fedId", &fedId);
          params.add("fedSize", &fedSize);
          params.add("fedSizeStdDev", &fedSizeStdDev);
          params.add("minFedSize", &minFedSize);
          params.add("maxFedSize", &maxFedSize);
          params.add("usePlayback", &usePlayback);
          params.add("playbackDataFile", &playbackDataFile);
          params.add("frameSize", &frameSize);
          params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
        }
      };

      typedef boost::shared_ptr<Configuration> ConfigurationPtr;

    } } } // namespace evb::test::dummyFEROL

#endif // _evb_test_dummyFEROL_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
