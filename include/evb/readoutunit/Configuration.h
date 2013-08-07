#ifndef _evb_readoutunit_configuration_h_
#define _evb_readoutunit_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/InfoSpaceItems.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"


namespace evb {

  namespace readoutunit {

    /**
    * \ingroup xdaqApps
    * \brief Configuration for the readout units (EVM/RU)
    */
    struct Configuration
    {
      xdata::String inputSource;                             // Input mode selection: FEROL or Local
      xdata::UnsignedInteger32 numberOfResponders;           // Number of threads handling responses to BUs
      xdata::UnsignedInteger32 blockSize;                    // I2O block size used for sending events to BUs
      xdata::UnsignedInteger32 fragmentFIFOCapacity;         // Capacity of the FIFO used to store FED data fragments
      xdata::UnsignedInteger32 fragmentRequestFIFOCapacity;  // Capacity of the FIFO to store incoming fragment requests
      xdata::Boolean dumpFragmentsToLogger;                  // If set to true, the incoming fragments are dumped to the logger
      xdata::Boolean dropInputData;                          // If set to true, the input data is dropped
      xdata::Boolean usePlayback;                            // Playback data from a file (not implemented)
      xdata::String playbackDataFile;                        // Path to the file used for data playback (not implemented)
      xdata::UnsignedInteger32 dummyFedSize;                 // Mean size in Bytes of the FED when running in Local mode
      xdata::Boolean useLogNormal;                           // If set to true, use the log-normal generator for FED sizes
      xdata::UnsignedInteger32 dummyFedSizeStdDev;           // Standard deviation of the FED sizes when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyFedSizeMin;              // Minimum size of the FED data when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyFedSizeMax;              // Maximum size of the FED data when using the log-normal distrubution
      xdata::UnsignedInteger32 fragmentPoolSize;             // Size of the toolbox::mem::Pool in Bytes used for dummy events
      xdata::UnsignedInteger32 frameSize;                    // The frame size in Bytes used for dummy events
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;  // Vector of FED ids
      xdata::UnsignedInteger32 maxTriggerAgeMSec;            // Maximum time in milliseconds before sending a response to event requests

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
        useLogNormal(false),
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
        params.add("useLogNormal", &useLogNormal);
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
