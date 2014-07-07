#ifndef _evb_readoutunit_configuration_h_
#define _evb_readoutunit_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"
#include "evb/UnsignedInteger32Less.h"
#include "xdaq/ApplicationContext.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
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
      xdata::String sendPoolName;                            // The pool name used for evb messages
      xdata::String inputSource;                             // Input mode selection: FEROL or Local
      xdata::UnsignedInteger32 numberOfResponders;           // Number of threads handling responses to BUs
      xdata::UnsignedInteger32 blockSize;                    // I2O block size used for sending events to BUs
      xdata::UnsignedInteger32 numberOfPreallocatedBlocks;   // Number of blocks pre-allocated during configure
      xdata::UnsignedInteger32 fragmentFIFOCapacity;         // Capacity of the FIFO used to store FED data fragments
      xdata::UnsignedInteger32 fragmentRequestFIFOCapacity;  // Capacity of the FIFO to store incoming fragment requests
      xdata::Boolean checkCRC;                               // If set to true, check the CRC of the FED fragments
      xdata::Boolean writeFragmentsToFile;                   // If set to true, the incoming fragments are written to text files
      xdata::UnsignedInteger32 writeNextFragmentsToFile;     // Write the next N fragments to text files
      xdata::Boolean dropInputData;                          // If set to true, the input data is dropped
      xdata::Boolean computeCRC;                             // If set to true, compute the CRC checksum of the dummy fragment
      xdata::Boolean usePlayback;                            // Playback data from a file (not implemented)
      xdata::String playbackDataFile;                        // Path to the file used for data playback (not implemented)
      xdata::UnsignedInteger32 dummyFedSize;                 // Mean size in Bytes of the FED when running in Local mode
      xdata::Boolean useLogNormal;                           // If set to true, use the log-normal generator for FED sizes
      xdata::UnsignedInteger32 dummyFedSizeStdDev;           // Standard deviation of the FED sizes when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyFedSizeMin;              // Minimum size of the FED data when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyFedSizeMax;              // Maximum size of the FED data when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyScalFedSize;             // Size in Bytes of the dummy SCAL. 0 disables it.
      xdata::UnsignedInteger32 scalFedId;                    // The FED id for the scaler information
      xdata::UnsignedInteger32 fragmentPoolSize;             // Size of the toolbox::mem::Pool in Bytes used for dummy events
      xdata::UnsignedInteger32 frameSize;                    // The frame size in Bytes used for dummy events
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;  // Vector of FED ids
      xdata::Vector<xdata::UnsignedInteger32> ruInstances;   // Vector of RU instances served from the EVM
      xdata::UnsignedInteger32 maxTriggerAgeMSec;            // Maximum time in milliseconds before sending a response to event requests
      xdata::UnsignedInteger32 fakeLumiSectionDuration;      // Duration in seconds of a fake luminosity section. If 0, don't generate lumi sections
      xdata::Double maxFedErrorRate;                         // Tolerated rate in Hz of FED errors

      Configuration()
        : sendPoolName("sudapl"),
          inputSource("FEROL"),
          numberOfResponders(4),
          blockSize(65536),
          numberOfPreallocatedBlocks(0),
          fragmentFIFOCapacity(128),
          fragmentRequestFIFOCapacity(64*18), // 64 BUs with 18 requests
          checkCRC(false),
          writeFragmentsToFile(false),
          writeNextFragmentsToFile(0),
          dropInputData(false),
          computeCRC(true),
          usePlayback(false),
          playbackDataFile(""),
          dummyFedSize(2048),
          useLogNormal(false),
          dummyFedSizeStdDev(0),
          dummyFedSizeMin(8), // minimum is 8 Bytes
          dummyFedSizeMax(0), // no limitation
          dummyScalFedSize(0),
          scalFedId(999),
          fragmentPoolSize(200000000),
          frameSize(32768),
          maxTriggerAgeMSec(1000),
          fakeLumiSectionDuration(0),
          maxFedErrorRate(1)
      {};

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        const uint32_t instance,
        xdaq::ApplicationContext* context
      )
      {
        fillDefaultFedSourceIds(instance);
        fillDefaultRUinstances(instance,context);

        params.add("sendPoolName", &sendPoolName);
        params.add("inputSource", &inputSource, InfoSpaceItems::change);
        params.add("numberOfResponders", &numberOfResponders);
        params.add("blockSize", &blockSize);
        params.add("numberOfPreallocatedBlocks", &numberOfPreallocatedBlocks);
        params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
        params.add("fragmentRequestFIFOCapacity", &fragmentRequestFIFOCapacity);
        params.add("checkCRC", &checkCRC);
        params.add("writeFragmentsToFile", &writeFragmentsToFile);
        params.add("writeNextFragmentsToFile", &writeNextFragmentsToFile, InfoSpaceItems::change);
        params.add("dropInputData", &dropInputData);
        params.add("computeCRC", &computeCRC);
        params.add("usePlayback", &usePlayback);
        params.add("playbackDataFile", &playbackDataFile);
        params.add("dummyFedSize", &dummyFedSize);
        params.add("useLogNormal", &useLogNormal);
        params.add("dummyFedSizeStdDev", &dummyFedSizeStdDev);
        params.add("dummyFedSizeMin", &dummyFedSizeMin);
        params.add("dummyFedSizeMax", &dummyFedSizeMax);
        params.add("dummyScalFedSize", &dummyScalFedSize);
        params.add("scalFedId", &scalFedId);
        params.add("fragmentPoolSize", &fragmentPoolSize);
        params.add("frameSize", &frameSize);
        params.add("fedSourceIds", &fedSourceIds);
        params.add("ruInstances", &ruInstances);
        params.add("maxTriggerAgeMSec", &maxTriggerAgeMSec);
        params.add("fakeLumiSectionDuration", &fakeLumiSectionDuration);
        params.add("maxFedErrorRate", &maxFedErrorRate);
      }

      void fillDefaultFedSourceIds(const uint32_t instance)
      {
        fedSourceIds.clear();

        // Default is 12 FEDs per super-fragment
        // RU0 has 0 to 11, RU1 has 12 to 23, etc.
        const uint32_t firstSourceId = (instance * 12);
        const uint32_t lastSourceId  = (instance * 12) + 11;
        for (uint32_t sourceId=firstSourceId; sourceId<=lastSourceId; ++sourceId)
        {
          fedSourceIds.push_back(sourceId);
        }
      }

      void fillDefaultRUinstances(const uint32_t instance, xdaq::ApplicationContext* context)
      {
        ruInstances.clear();

        std::set<xdaq::ApplicationDescriptor*> ruDescriptors;

        try
        {
          ruDescriptors =
            context->getDefaultZone()->
            getApplicationDescriptors("evb::RU");
        }
        catch(xcept::Exception& e)
        {
          XCEPT_RETHROW(exception::I2O,
                        "Failed to get RU application descriptor", e);
        }

        for (std::set<xdaq::ApplicationDescriptor*>::const_iterator
               it=ruDescriptors.begin(), itEnd =ruDescriptors.end();
             it != itEnd; ++it)
        {
          ruInstances.push_back((*it)->getInstance());
        }

        std::sort(ruInstances.begin(), ruInstances.end(),
                  UnsignedInteger32Less());
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
