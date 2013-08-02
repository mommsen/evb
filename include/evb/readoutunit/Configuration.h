#ifndef _evb_readoutunit_configuration_h_
#define _evb_readoutunit_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/InfoSpaceItems.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"


namespace evb {

  namespace readoutunit {

    /**
    * \ingroup xdaqApps
    * \brief Configuration for the readout units (EVM/RU)
    */
    struct Configuration
    {
      xdata::String inputSource;
      xdata::UnsignedInteger32 numberOfResponders;
      xdata::UnsignedInteger32 blockSize;
      xdata::UnsignedInteger32 fragmentFIFOCapacity;
      xdata::UnsignedInteger32 fragmentRequestFIFOCapacity;
      xdata::Boolean dumpFragmentsToLogger;
      xdata::Boolean dropInputData;
      xdata::Boolean usePlayback;
      xdata::String playbackDataFile;
      xdata::UnsignedInteger32 dummyFedSize;
      xdata::UnsignedInteger32 dummyFedSizeStdDev;
      xdata::UnsignedInteger32 dummyFedSizeMin;
      xdata::UnsignedInteger32 dummyFedSizeMax;
      xdata::UnsignedInteger32 fragmentPoolSize;
      xdata::UnsignedInteger32 frameSize;
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;
      xdata::UnsignedInteger32 maxTriggerAgeMSec;

      Configuration()
      : inputSource("FEROL"),
        numberOfResponders(1),
        blockSize(32768),
        fragmentFIFOCapacity(128),
        fragmentRequestFIFOCapacity(80*8), // 80 BUs with max 8 request
        dumpFragmentsToLogger(false),
        dropInputData(false),
        usePlayback(false),
        playbackDataFile(""),
        dummyFedSize(2048),
        dummyFedSizeStdDev(0),
        dummyFedSizeMin(8), // minimum is 8 Bytes
        dummyFedSizeMax(0), // no limitation
        fragmentPoolSize(10000000),
        frameSize(32768),
        maxTriggerAgeMSec(1000)
      {};

      void addToInfoSpace(InfoSpaceItems& params, const uint32_t instance)
      {
        // Default is 8 FEDs per super-fragment
        // RU0 has 0 to 7, RU1 has 8 to 15, etc.
        const uint32_t firstSourceId = (instance * 8);
        const uint32_t lastSourceId  = (instance * 8) + 7;
        for (uint32_t sourceId=firstSourceId; sourceId<=lastSourceId; ++sourceId)
        {
          fedSourceIds.push_back(sourceId);
        }

        params.add("inputSource", &inputSource, InfoSpaceItems::change);
        params.add("numberOfResponders", &numberOfResponders);
        params.add("blockSize", &blockSize);
        params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
        params.add("fragmentRequestFIFOCapacity", &fragmentRequestFIFOCapacity);
        params.add("dumpFragmentsToLogger", &dumpFragmentsToLogger);
        params.add("dropInputData", &dropInputData);
        params.add("usePlayback", &usePlayback);
        params.add("playbackDataFile", &playbackDataFile);
        params.add("dummyFedSize", &dummyFedSize);
        params.add("dummyFedSizeStdDev", &dummyFedSizeStdDev);
        params.add("dummyFedSizeMin", &dummyFedSizeMin);
        params.add("dummyFedSizeMax", &dummyFedSizeMax);
        params.add("fragmentPoolSize", &fragmentPoolSize);
        params.add("frameSize", &frameSize);
        params.add("fedSourceIds", &fedSourceIds);
        params.add("maxTriggerAgeMSec", &maxTriggerAgeMSec);
      }
    };

    typedef boost::shared_ptr<Configuration> ConfigurationPtr;

  } } // namespace evb::readoutunit

#endif // _evb_readoutunit_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
