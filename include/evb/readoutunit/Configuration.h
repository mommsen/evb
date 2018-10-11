#ifndef _evb_readoutunit_configuration_h_
#define _evb_readoutunit_configuration_h_

#include <memory>
#include <stdint.h>

#include "evb/Constants.h"
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
        xdata::UnsignedInteger32 dummyFedSize;
        xdata::UnsignedInteger32 dummyFedSizeStdDev;
        xdata::Boolean active;
        std::string ipAddress;

        FerolSource() :
          fedId(FED_COUNT+1),hostname(""),port(0),dummyFedSize(2048),dummyFedSizeStdDev(0),active(false),ipAddress("") {};

        const std::string& getIPaddress()
        {
          if ( ipAddress.empty() )
          {
            try
            {
              ipAddress = resolveIPaddress(hostname);
            }
            catch(const char* error) {
              std::ostringstream msg;
              msg << "Failed to resolve hostname " << hostname.toString();
              msg << " : " << error;
              XCEPT_RAISE(exception::TCP,msg.str());
            }
          }
          return ipAddress;
        }

        void registerFields(xdata::Bag<FerolSource>* bag)
        {
          bag->addField("fedId",&fedId);
          bag->addField("hostname",&hostname);
          bag->addField("port",&port);
          bag->addField("dummyFedSize",&dummyFedSize);
          bag->addField("dummyFedSizeStdDev",&dummyFedSizeStdDev);
        }
      };
      using FerolSources = xdata::Vector< xdata::Bag<FerolSource> >;

      xdata::String sendPoolName;                            // The pool name used for evb messages
      xdata::String inputSource;                             // Input mode selection: Socket or Local
      xdata::UnsignedInteger32 numberOfResponders;           // Number of threads handling responses to BUs
      xdata::UnsignedInteger32 blockSize;                    // I2O block size used for sending events to BUs
      xdata::UnsignedInteger32 numberOfPreallocatedBlocks;   // Number of blocks pre-allocated during configure
      xdata::UnsignedInteger32 socketBufferFIFOCapacity;     // Capacity of the FIFO used to store socket buffers per FEROL
      xdata::UnsignedInteger32 grantFIFOCapacity;            // Capacity of the FIFO used to grant buffers per pipe
      xdata::UnsignedInteger32 fragmentFIFOCapacity;         // Capacity of the FIFO used to store FED data fragments
      xdata::UnsignedInteger32 fragmentRequestFIFOCapacity;  // Capacity of the FIFO to store incoming fragment requests
      xdata::UnsignedInteger32 checkCRC;                     // Check the CRC of the FED fragments for every Nth event
      xdata::UnsignedInteger32 writeNextFragmentsToFile;     // Write the next N fragments to text files
      xdata::Boolean dropAtSocket;                           // If set to true, data is discarded after reading from the socket
      xdata::Boolean dropInputData;                          // If set to true, the input data is dropped
      xdata::Boolean computeCRC;                             // If set to true, compute the CRC checksum of the dummy fragment
      xdata::Boolean usePlayback;                            // Playback data from a file (not implemented)
      xdata::String playbackDataFile;                        // Path to the file used for data playback (not implemented)
      xdata::UnsignedInteger32 dummyFedSizeMin;              // Minimum size of the FED data when using the log-normal distrubution
      xdata::UnsignedInteger32 dummyFedSizeMax;              // Maximum size of the FED data when using the log-normal distrubution
      xdata::String dipNodes;                                // Comma-separated list of DIP nodes
      xdata::String maskedDipTopics;                         // DIP topics which will not be considered
      xdata::UnsignedInteger32 fragmentPoolSize;             // Size of the toolbox::mem::Pool in Bytes used for dummy events
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;  // Vector of activ FED ids
      FerolSources ferolSources;                             // Vector of FEROL sources
      xdata::Boolean createSoftFed1022;                      // Creats softFED 1022 using the meta data from DIP
      xdata::Boolean checkBxId;                              // Check if the BX ids match when building super fragments
      xdata::Boolean tolerateCorruptedEvents;                // Tolerate corrupted FED data (excluding CRC errors)
      xdata::Boolean tolerateOutOfSequenceEvents;            // Tolerate events out of sequence
      xdata::UnsignedInteger32 maxDumpsPerFED;               // Maximum number of fragment dumps per FED and run
      xdata::UnsignedInteger32 ferolConnectTimeOut;          // Timeout in seconds when waiting for FEROL connections
      xdata::UnsignedInteger32 maxTimeWithIncompleteEvents;  // Build incomplete events for at most this time in seconds
      xdata::UnsignedInteger32 maxPostRetries;               // Max. attempts to post an I2O message


      Configuration()
        : sendPoolName("sudapl"),
          inputSource("Socket"),
          numberOfResponders(6),
          blockSize(65536),
          numberOfPreallocatedBlocks(0),
          socketBufferFIFOCapacity(128),
          grantFIFOCapacity(4096),
          fragmentFIFOCapacity(512),
          fragmentRequestFIFOCapacity(2048),
          checkCRC(1),
          writeNextFragmentsToFile(0),
          dropAtSocket(false),
          dropInputData(false),
          computeCRC(true),
          usePlayback(false),
          playbackDataFile(""),
          dummyFedSizeMin(16), // minimum is 16 Bytes
          dummyFedSizeMax(0), // no limitation
          dipNodes("cmsdimns1.cern.ch,cmsdimns2.cern.ch"),
          maskedDipTopics(""),
          fragmentPoolSize(200000000),
          createSoftFed1022(false),
          checkBxId(true),
          tolerateCorruptedEvents(false),
          tolerateOutOfSequenceEvents(false),
          maxDumpsPerFED(10),
          ferolConnectTimeOut(5),
          maxTimeWithIncompleteEvents(60),
          maxPostRetries(10)
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
        params.add("grantFIFOCapacity", &grantFIFOCapacity);
        params.add("fragmentFIFOCapacity", &fragmentFIFOCapacity);
        params.add("fragmentRequestFIFOCapacity", &fragmentRequestFIFOCapacity);
        params.add("checkCRC", &checkCRC);
        params.add("writeNextFragmentsToFile", &writeNextFragmentsToFile, InfoSpaceItems::change);
        params.add("dropAtSocket", &dropAtSocket);
        params.add("dropInputData", &dropInputData);
        params.add("computeCRC", &computeCRC);
        params.add("usePlayback", &usePlayback);
        params.add("playbackDataFile", &playbackDataFile);
        params.add("dummyFedSizeMin", &dummyFedSizeMin);
        params.add("dummyFedSizeMax", &dummyFedSizeMax);
        params.add("dipNodes", &dipNodes);
        params.add("maskedDipTopics", &maskedDipTopics);
        params.add("fragmentPoolSize", &fragmentPoolSize);
        params.add("fedSourceIds", &fedSourceIds);
        params.add("ferolSources", &ferolSources);
        params.add("createSoftFed1022", &createSoftFed1022);
        params.add("checkBxId", &checkBxId);
        params.add("tolerateCorruptedEvents", &tolerateCorruptedEvents);
        params.add("tolerateOutOfSequenceEvents", &tolerateOutOfSequenceEvents);
        params.add("maxDumpsPerFED", &maxDumpsPerFED);
        params.add("ferolConnectTimeOut", &ferolConnectTimeOut);
        params.add("maxTimeWithIncompleteEvents", &maxTimeWithIncompleteEvents);
        params.add("maxPostRetries", &maxPostRetries);
      }

    };

    using ConfigurationPtr = std::shared_ptr<Configuration>;

  } } // namespace evb::readoutunit

#endif // _evb_readoutunit_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
