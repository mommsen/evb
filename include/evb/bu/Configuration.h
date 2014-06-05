#ifndef _evb_bu_configuration_h_
#define _evb_bu_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/InfoSpaceItems.h"
#include "xdaq/ApplicationContext.h"
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
      xdata::String sendPoolName;                          // The pool name used for evb messages
      xdata::Integer32 evmInstance;                        // Instance of the EVM. If not set, discover the EVM over I2O.
      xdata::UnsignedInteger32 maxEvtsUnderConstruction;   // Maximum number of events in BU
      xdata::UnsignedInteger32 eventsPerRequest;           // Number of events requested at a time
      xdata::Double resourcesPerCore;                      // Number of resource IDs per active FU core
      xdata::UnsignedInteger32 staleResourceTime;          // Number of seconds after which a FU resource is no longer considered
      xdata::UnsignedInteger32 superFragmentFIFOCapacity;  // Capacity of the FIFO for super-fragment
      xdata::Boolean dropEventData;                        // If true, drop the data as soon as the event is complete
      xdata::UnsignedInteger32 numberOfBuilders;           // Number of threads used to build/write events
      xdata::String rawDataDir;                            // Path to the top directory used to write the event data
      xdata::String metaDataDir;                           // Path to the top directory used to write the meta data (JSON)
      xdata::String jsdDirName;                            // Directory name under the run directory used for JSON definition files
      xdata::String hltDirName;                            // Directory name under the run directory used for HLT configuration files
      xdata::String fuLockName;                            // Filename of the lock file used to arbritrate file-access btw FUs
      xdata::Double rawDataHighWaterMark;                  // Relative high-water mark for the event data directory
      xdata::Double rawDataLowWaterMark;
      xdata::Double metaDataHighWaterMark;
      xdata::Double metaDataLowWaterMark;
      xdata::Boolean checkCRC;                             // If set to true, check the CRC of the FED fragments
      xdata::Boolean calculateAdler32;                     // If set to true, an adler32 checksum of data blob of each event is calculated
      xdata::Boolean deleteRawDataFiles;                   // If true, delete raw data files when the high-water mark is reached
      xdata::UnsignedInteger32 maxEventsPerFile;           // Maximum number of events written into one file
      xdata::UnsignedInteger32 sleepBetweenEvents;         // Throttle output rate by sleeping us between writing events
      xdata::UnsignedInteger32 fileStatisticsFIFOCapacity; // Capacity of the FIFO used for file accounting
      xdata::UnsignedInteger32 lumiSectionFIFOCapacity;    // Capacity of the FIFO used for lumi-section accounting
      xdata::UnsignedInteger32 lumiSectionTimeout;         // Time in seconds after which a lumi-section is considered complete
      xdata::String hltParameterSetURL;                    // URL of the HLT menu

      Configuration()
        : sendPoolName("sudapl"),
          evmInstance(-1), // Explicitly indicate parameter not set
          maxEvtsUnderConstruction(6*3*16), // 6 builders with 3 requests for 16 events
          eventsPerRequest(16),
          resourcesPerCore(0.1),
          staleResourceTime(10),
          superFragmentFIFOCapacity(16384),
          dropEventData(false),
          numberOfBuilders(6),
          rawDataDir("/tmp/fff"),
          metaDataDir("/tmp/fff"),
          jsdDirName("jsd"),
          hltDirName("hlt"),
          fuLockName("fu.lock"),
          rawDataHighWaterMark(0.7),
          rawDataLowWaterMark(0.5),
          metaDataHighWaterMark(0.9),
          metaDataLowWaterMark(0.5),
          checkCRC(true),
          calculateAdler32(false),
          deleteRawDataFiles(false),
          maxEventsPerFile(2000),
          sleepBetweenEvents(0),
          fileStatisticsFIFOCapacity(128),
          lumiSectionFIFOCapacity(128),
          lumiSectionTimeout(25),
          hltParameterSetURL("")
      {};

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        const uint32_t instance,
        xdaq::ApplicationContext* context
      )
      {
        params.add("sendPoolName", &sendPoolName);
        params.add("evmInstance", &evmInstance);
        params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction);
        params.add("eventsPerRequest", &eventsPerRequest);
        params.add("resourcesPerCore", &resourcesPerCore);
        params.add("staleResourceTime", &staleResourceTime);
        params.add("superFragmentFIFOCapacity", &superFragmentFIFOCapacity);
        params.add("dropEventData", &dropEventData);
        params.add("numberOfBuilders", &numberOfBuilders);
        params.add("rawDataDir", &rawDataDir);
        params.add("metaDataDir", &metaDataDir);
        params.add("jsdDirName", &jsdDirName);
        params.add("hltDirName", &hltDirName);
        params.add("fuLockName", &fuLockName);
        params.add("rawDataHighWaterMark", &rawDataHighWaterMark);
        params.add("rawDataLowWaterMark", &rawDataLowWaterMark);
        params.add("metaDataHighWaterMark", &metaDataHighWaterMark);
        params.add("metaDataLowWaterMark", &metaDataLowWaterMark);
        params.add("checkCRC", &checkCRC);
        params.add("calculateAdler32", &calculateAdler32);
        params.add("deleteRawDataFiles", &deleteRawDataFiles);
        params.add("maxEventsPerFile", &maxEventsPerFile);
        params.add("sleepBetweenEvents", &sleepBetweenEvents);
        params.add("fileStatisticsFIFOCapacity", &fileStatisticsFIFOCapacity);
        params.add("lumiSectionFIFOCapacity", &lumiSectionFIFOCapacity);
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
