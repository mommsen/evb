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
#include "xdata/Vector.h"


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
      xdata::UnsignedInteger32 sleepTimeBlocked;           // Time to sleep in ns for each blocked resource
      xdata::UnsignedInteger32 maxRequestRate;             // Maximum rate in Hz to send request to EVM
      xdata::Double fuOutputBandwidthLow;                  // Low water mark on bandwidth used for output of FUs
      xdata::Double fuOutputBandwidthHigh;                 // High water mark on bandwidth used for output of FUs
      xdata::UnsignedInteger32 lumiSectionLatencyLow;      // Low water mark on how many LS may be queued for the FUs
      xdata::UnsignedInteger32 lumiSectionLatencyHigh;     // High water mark on how many LS may be queued for the FUs
      xdata::UnsignedInteger32 maxFuLumiSectionLatency;    // Maximum number of lumi sections the FUs may lag behind
      xdata::UnsignedInteger32 maxTriesFUsStale;           // Maximum number of consecutive tests for FU staleness before failing
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
      xdata::Boolean calculateCRC32c;                      // If set to true, a CRC32c checksum of data blob of each event is calculated
      xdata::Boolean deleteRawDataFiles;                   // If true, delete raw data files when the high-water mark is reached
      xdata::Boolean ignoreResourceSummary;                // If true, ignore the resource_summary file from hltd
      xdata::Boolean usePriorities;                        // If true, prioritize the event requests to the EVM
      xdata::UnsignedInteger32 maxEventsPerFile;           // Maximum number of events written into one file
      xdata::UnsignedInteger32 fileStatisticsFIFOCapacity; // Capacity of the FIFO used for file accounting
      xdata::UnsignedInteger32 lumiSectionFIFOCapacity;    // Capacity of the FIFO used for lumi-section accounting
      xdata::UnsignedInteger32 lumiSectionTimeout;         // Time in seconds after which a lumi-section is considered complete
      xdata::String hltParameterSetURL;                    // URL of the HLT menu
      xdata::Vector<xdata::String> hltFiles;               // List of file names to retrieve from hltParameterSetURL
      xdata::String blacklistName;                         // Name of the blacklist file
      xdata::String fuBlacklist;                           // The FUs to blacklist as string
      xdata::UnsignedInteger32 roundTripTimeSamples;       // Rolling average of round trip times for the last N I2O mesage (0 disables it)
      xdata::UnsignedInteger32 maxPostRetries;             // Max. attempts to post an I2O message


      Configuration()
        : sendPoolName("sudapl"),
          evmInstance(-1), // Explicitly indicate parameter not set
          maxEvtsUnderConstruction(256),
          eventsPerRequest(8),
          resourcesPerCore(0.2),
          sleepTimeBlocked(5000),
          maxRequestRate(1000),
          fuOutputBandwidthLow(100),
          fuOutputBandwidthHigh(120),
          lumiSectionLatencyLow(1),
          lumiSectionLatencyHigh(4),
          maxFuLumiSectionLatency(3),
          maxTriesFUsStale(60),
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
          calculateCRC32c(true),
          deleteRawDataFiles(false),
          ignoreResourceSummary(false),
          usePriorities(true),
          maxEventsPerFile(400),
          fileStatisticsFIFOCapacity(128),
          lumiSectionFIFOCapacity(128),
          lumiSectionTimeout(30),
          hltParameterSetURL(""),
          blacklistName("blacklist"),
          fuBlacklist("[]"),
          roundTripTimeSamples(1000),
          maxPostRetries(10)
      {
        hltFiles.push_back("HltConfig.py");
        hltFiles.push_back("fffParameters.jsn");
      };

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        xdaq::ApplicationContext* context
      )
      {
        params.add("sendPoolName", &sendPoolName);
        params.add("evmInstance", &evmInstance);
        params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction);
        params.add("eventsPerRequest", &eventsPerRequest);
        params.add("resourcesPerCore", &resourcesPerCore);
        params.add("sleepTimeBlocked", &sleepTimeBlocked);
        params.add("maxRequestRate", &maxRequestRate);
        params.add("fuOutputBandwidthLow", &fuOutputBandwidthLow);
        params.add("fuOutputBandwidthHigh", &fuOutputBandwidthHigh);
        params.add("lumiSectionLatencyLow", &lumiSectionLatencyLow);
        params.add("lumiSectionLatencyHigh", &lumiSectionLatencyHigh);
        params.add("maxFuLumiSectionLatency", &maxFuLumiSectionLatency);
        params.add("maxTriesFUsStale", &maxTriesFUsStale);
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
        params.add("calculateCRC32c", &calculateCRC32c);
        params.add("deleteRawDataFiles", &deleteRawDataFiles);
        params.add("ignoreResourceSummary", &ignoreResourceSummary);
        params.add("usePriorities", &usePriorities);
        params.add("maxEventsPerFile", &maxEventsPerFile);
        params.add("fileStatisticsFIFOCapacity", &fileStatisticsFIFOCapacity);
        params.add("lumiSectionFIFOCapacity", &lumiSectionFIFOCapacity);
        params.add("lumiSectionTimeout", &lumiSectionTimeout);
        params.add("hltParameterSetURL", &hltParameterSetURL);
        params.add("hltFiles", &hltFiles);
        params.add("blacklistName", &blacklistName);
        params.add("fuBlacklist", &fuBlacklist);
        params.add("roundTripTimeSamples", &roundTripTimeSamples);
        params.add("maxPostRetries", &maxPostRetries);
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
