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
      xdata::UnsignedInteger32 maxFuLumiSectionLatency;    // Maximum number of lumi sections the FUs may lag behind
      xdata::UnsignedInteger32 staleResourceTime;          // Number of seconds after which a FU resource is no longer considered
      xdata::UnsignedInteger32 superFragmentFIFOCapacity;  // Capacity of the FIFO for super-fragment
      xdata::Boolean dropEventData;                        // If true, drop the data as soon as the event is complete
      xdata::UnsignedInteger32 numberOfBuilders;           // Number of threads used to build/write events
      xdata::String rawDataDir;                            // Path to the top directory used to write the event data
      xdata::String metaDataDir;                           // Path to the top directory used to write the meta data (JSON)
      xdata::String jsdDirName;                            // Directory name under the run directory used for JSON definition files
      xdata::String hltDirName;                            // Directory name under the run directory used for HLT configuration files
      xdata::String fuLockName;                            // Filename of the lock file used to arbritrate file-access btw FUs
      xdata::String resourceSummaryFileName;               // Relative path to resource summary file
      xdata::Double rawDataHighWaterMark;                  // Relative high-water mark for the event data directory
      xdata::Double rawDataLowWaterMark;
      xdata::Double metaDataHighWaterMark;
      xdata::Double metaDataLowWaterMark;
      xdata::UnsignedInteger32 checkCRC;                   // Check the CRC of the FED fragments for every Nth event
      xdata::Boolean calculateAdler32;                     // If set to true, an adler32 checksum of data blob of each event is calculated
      xdata::Boolean deleteRawDataFiles;                   // If true, delete raw data files when the high-water mark is reached
      xdata::Boolean ignoreResourceSummary;                // If true, ignore the resource_summary file from hltd
      xdata::UnsignedInteger32 maxEventsPerFile;           // Maximum number of events written into one file
      xdata::UnsignedInteger32 fileStatisticsFIFOCapacity; // Capacity of the FIFO used for file accounting
      xdata::UnsignedInteger32 lumiSectionFIFOCapacity;    // Capacity of the FIFO used for lumi-section accounting
      xdata::UnsignedInteger32 lumiSectionTimeout;         // Time in seconds after which a lumi-section is considered complete
      xdata::String hltParameterSetURL;                    // URL of the HLT menu

      Configuration()
        : sendPoolName("sudapl"),
          evmInstance(-1), // Explicitly indicate parameter not set
          maxEvtsUnderConstruction(256),
          eventsPerRequest(8),
          resourcesPerCore(0.2),
          maxFuLumiSectionLatency(3),
          staleResourceTime(10),
          superFragmentFIFOCapacity(3072),
          dropEventData(false),
          numberOfBuilders(6),
          rawDataDir("/tmp/fff"),
          metaDataDir("/tmp/fff"),
          jsdDirName("jsd"),
          hltDirName("hlt"),
          fuLockName("fu.lock"),
          resourceSummaryFileName("appliance/resource_summary"),
          rawDataHighWaterMark(0.95),
          rawDataLowWaterMark(0.75),
          metaDataHighWaterMark(0.95),
          metaDataLowWaterMark(0.75),
          checkCRC(1),
          calculateAdler32(true),
          deleteRawDataFiles(false),
          ignoreResourceSummary(false),
          maxEventsPerFile(400),
          fileStatisticsFIFOCapacity(128),
          lumiSectionFIFOCapacity(128),
          lumiSectionTimeout(30),
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
        params.add("maxFuLumiSectionLatency", &maxFuLumiSectionLatency);
        params.add("staleResourceTime", &staleResourceTime);
        params.add("superFragmentFIFOCapacity", &superFragmentFIFOCapacity);
        params.add("dropEventData", &dropEventData);
        params.add("numberOfBuilders", &numberOfBuilders);
        params.add("rawDataDir", &rawDataDir);
        params.add("metaDataDir", &metaDataDir);
        params.add("jsdDirName", &jsdDirName);
        params.add("hltDirName", &hltDirName);
        params.add("fuLockName", &fuLockName);
        params.add("resourceSummaryFileName", &resourceSummaryFileName);
        params.add("rawDataHighWaterMark", &rawDataHighWaterMark);
        params.add("rawDataLowWaterMark", &rawDataLowWaterMark);
        params.add("metaDataHighWaterMark", &metaDataHighWaterMark);
        params.add("metaDataLowWaterMark", &metaDataLowWaterMark);
        params.add("checkCRC", &checkCRC);
        params.add("calculateAdler32", &calculateAdler32);
        params.add("deleteRawDataFiles", &deleteRawDataFiles);
        params.add("ignoreResourceSummary", &ignoreResourceSummary);
        params.add("maxEventsPerFile", &maxEventsPerFile);
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
