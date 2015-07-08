#ifndef _evb_readoutunit_configuration_h_
#define _evb_readoutunit_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"
#include "xdaq/ApplicationContext.h"
#include "xdata/Bag.h"
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
      struct FerolSource
      {
        xdata::UnsignedInteger32 fedId;
        xdata::String hostname;
        xdata::UnsignedInteger32 port;
        xdata::Boolean active;

        void registerFields(xdata::Bag<FerolSource>* bag)
        {
          bag->addField("fedId",&fedId);
          bag->addField("hostname",&hostname);
          bag->addField("port",&port);
        }
      };
      typedef xdata::Vector< xdata::Bag<FerolSource> > FerolSources;

      xdata::String sendPoolName;                            // The pool name used for evb messages
      xdata::String inputSource;                             // Input mode selection: FEROL or Local
      xdata::UnsignedInteger32 numberOfResponders;           // Number of threads handling responses to BUs
      xdata::UnsignedInteger32 blockSize;                    // I2O block size used for sending events to BUs
      xdata::UnsignedInteger32 numberOfPreallocatedBlocks;   // Number of blocks pre-allocated during configure
      xdata::UnsignedInteger32 socketBufferFIFOCapacity;     // Capacity of the FIFO used to store socket buffers
      xdata::UnsignedInteger32 fragmentFIFOCapacity;         // Capacity of the FIFO used to store FED data fragments
      xdata::UnsignedInteger32 fragmentRequestFIFOCapacity;  // Capacity of the FIFO to store incoming fragment requests
      xdata::UnsignedInteger32 checkCRC;                     // Check the CRC of the FED fragments for every Nth event
      xdata::UnsignedInteger32 writeNextFragmentsToFile;     // Write the next N fragments to text files
      xdata::Boolean dropAtSocket;                           // If set to true, data is discarded after reading from the socket
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
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;  // Vector of activ FED ids
      FerolSources ferolSources;                             // Vector of FEROL sources
      xdata::Boolean tolerateCorruptedEvents;                // Tolerate corrupted FED data (excluding CRC errors)
      xdata::Double maxCRCErrorRate;                         // Tolerated rate in Hz of FED CRC errors
      xdata::UnsignedInteger32 maxDumpsPerFED;               // Maximum number of fragment dumps per FED and run

      Configuration()
        : sendPoolName("sudapl"),
          inputSource("FEROL"),
          numberOfResponders(4),
          blockSize(65536),
          numberOfPreallocatedBlocks(0),
          socketBufferFIFOCapacity(32),
          fragmentFIFOCapacity(128),
          fragmentRequestFIFOCapacity(2048), // 64 BUs with 32 requests
          checkCRC(1),
          writeNextFragmentsToFile(0),
          dropAtSocket(false),
          dropInputData(false),
          computeCRC(true),
          usePlayback(false),
          playbackDataFile(""),
          dummyFedSize(2048),
          useLogNormal(false),
          dummyFedSizeStdDev(0),
          dummyFedSizeMin(16), // minimum is 16 Bytes
          dummyFedSizeMax(0), // no limitation
          dummyScalFedSize(0),
          scalFedId(999),
          fragmentPoolSize(200000000),
          tolerateCorruptedEvents(false),
          maxCRCErrorRate(1000),
          maxDumpsPerFED(10)
      {};

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        xdaq::ApplicationContext* context
      )
      {
        params.add("sendPoolName", &sendPoolName);
        params.add("inputSource", &inputSource);
        params.add("numberOfResponders", &numberOfResponders);
        params.add("blockSize", &blockSize);
        params.add("numberOfPreallocatedBlocks", &numberOfPreallocatedBlocks);
        params.add("socketBufferFIFOCapacity", &socketBufferFIFOCapacity);
        params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
        params.add("fragmentRequestFIFOCapacity", &fragmentRequestFIFOCapacity);
        params.add("checkCRC", &checkCRC);
        params.add("writeNextFragmentsToFile", &writeNextFragmentsToFile, InfoSpaceItems::change);
        params.add("dropAtSocket", &dropAtSocket);
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
        params.add("fedSourceIds", &fedSourceIds);
        params.add("ferolSources", &ferolSources);
        params.add("tolerateCorruptedEvents", &tolerateCorruptedEvents);
        params.add("maxCRCErrorRate", &maxCRCErrorRate);
        params.add("maxDumpsPerFED", &maxDumpsPerFED);
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
