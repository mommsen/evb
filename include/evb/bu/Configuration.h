#ifndef _evb_bu_configuration_h_
#define _evb_bu_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/InfoSpaceItems.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/Integer32.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  namespace bu {

    /**
    * \ingroup xdaqApps
    * \brief Configuration for the builder unit (BU)
    */
    struct Configuration
    {
      xdata::Integer32 evmInstance;                        // Instance of the EVM. If not set, discover the EVM over I2O.
      xdata::UnsignedInteger32 maxEvtsUnderConstruction;   // Maximum number of events in BU
      xdata::UnsignedInteger32 eventsPerRequest;           // Number of events requested at a time
      xdata::UnsignedInteger32 superFragmentFIFOCapacity;  // Capacity of the FIFO for super-fragment
      xdata::Boolean dropEventData;                        // If true, drop the data as soon as the event is complete
      xdata::UnsignedInteger32 numberOfBuilders;           // Number of threads used to build/write events
      xdata::String rawDataDir;                            // Path to the top directory used to write the event data
      xdata::String metaDataDir;                           // Path to the top directory used to write the meta data (JSON)
      xdata::Double rawDataHighWaterMark;                  // Relative high-water mark for the event data directory
      xdata::Double rawDataLowWaterMark;
      xdata::Double metaDataHighWaterMark;
      xdata::Double metaDataLowWaterMark;
      xdata::Boolean checkCRC;                             // If set to true, check the CRC of the FED fragments
      xdata::Boolean deleteRawDataFiles;                   // If true, delete raw data files when the high-water mark is reached
      xdata::UnsignedInteger32 maxEventsPerFile;           // Maximum number of events written into one file
      xdata::UnsignedInteger32 lumiMonitorFIFOCapacity;    // Capacity of the FIFO used for lumi-section monitoring
      xdata::UnsignedInteger32 lumiSectionTimeout;         // Time in seconds after which a lumi-section is considered complete
      xdata::String hltParameterSetURL;                    // URL of the HLT menu

      Configuration()
      : evmInstance(-1), // Explicitly indicate parameter not set
        maxEvtsUnderConstruction(6*3*16), // 6 builders with 3 requests for 16 events
        eventsPerRequest(16),
        superFragmentFIFOCapacity(16384),
        dropEventData(false),
        numberOfBuilders(6),
        rawDataDir("/tmp/fff"),
        metaDataDir("/tmp/fff"),
        rawDataHighWaterMark(0.7),
        rawDataLowWaterMark(0.5),
        metaDataHighWaterMark(0.9),
        metaDataLowWaterMark(0.5),
        checkCRC(true),
        deleteRawDataFiles(false),
        maxEventsPerFile(2000),
        lumiMonitorFIFOCapacity(128),
        lumiSectionTimeout(25),
        hltParameterSetURL("file:///opt/hltd/python/testFU_cfg1.py")
      {};

      void addToInfoSpace(InfoSpaceItems& params, const uint32_t instance)
      {
        params.add("evmInstance", &evmInstance);
        params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction);
        params.add("eventsPerRequest", &eventsPerRequest);
        params.add("superFragmentFIFOCapacity", &superFragmentFIFOCapacity);
        params.add("dropEventData", &dropEventData);
        params.add("numberOfBuilders", &numberOfBuilders);
        params.add("rawDataDir", &rawDataDir);
        params.add("metaDataDir", &metaDataDir);
        params.add("rawDataHighWaterMark", &rawDataHighWaterMark);
        params.add("rawDataLowWaterMark", &rawDataLowWaterMark);
        params.add("metaDataHighWaterMark", &metaDataHighWaterMark);
        params.add("metaDataLowWaterMark", &metaDataLowWaterMark);
        params.add("checkCRC", &checkCRC);
        params.add("deleteRawDataFiles", &deleteRawDataFiles);
        params.add("maxEventsPerFile", &maxEventsPerFile);
        params.add("lumiMonitorFIFOCapacity", &lumiMonitorFIFOCapacity);
        params.add("lumiSectionTimeout", &lumiSectionTimeout);
        params.add("hltParameterSetURL", &hltParameterSetURL);
      }
    };

    typedef boost::shared_ptr<Configuration> ConfigurationPtr;

  } } // namespace evb::bu

#endif // _evb_bu_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
