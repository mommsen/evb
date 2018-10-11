#ifndef _evb_test_dummyFEROL_FragmentGenerator_h_
#define _evb_test_dummyFEROL_FragmentGenerator_h_

#include <stdint.h>
#include <string>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/FragmentTracker.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  namespace test {
    namespace dummyFEROL {

      /**
       * struct for lumi section and L1 trigger bit information
       */
      struct L1Information
      {
        bool isValid;
        char reason[100];
        uint32_t runNumber;
        uint32_t lsNumber;
        uint32_t bunchCrossing;
        uint32_t orbitNumber;
        uint16_t eventType;
        uint64_t l1Technical;
        uint64_t l1Decision_0_63;
        uint64_t l1Decision_64_127;

        L1Information();
        void reset();
        const L1Information& operator=(const L1Information&);
      };


      /**
       * \ingroup xdaqApps
       * \brief Provide dummy FED data
       */

      class FragmentGenerator
      {

      public:

        FragmentGenerator();

        ~FragmentGenerator() {};

        /**
         * Configure the super-fragment generator.
         * If usePlayback is set to true, the data is read from the playbackDataFile,
         * otherwise, dummy data is generated according to the fedPayloadSize.
         * The frameSize specifies the size of the data frames.
         */
        void configure
        (
          const uint32_t fedid,
          const bool usePlayback,
          const std::string& playbackDataFile,
          const uint32_t frameSize,
          const uint32_t fedSize,
          const bool computeCRC,
          const uint32_t fedSizeStdDev,
          const uint32_t minFedSize,
          const uint32_t maxFedSize,
          const size_t fragmentPoolSize,
          const uint32_t fakeLumiSectionDuration,
          const uint32_t maxTriggerRate
        );

        /**
         * Start serving events for a new run
         */
        void reset();

        /**
         * Get the next fragment.
         * Returns false if no fragment can be generated
         */
        bool getData(toolbox::mem::Reference*&,
                     const uint32_t stopAtEventNumber,
                     uint32_t& lastEventNumber,
                     uint32_t& skipNbEvents,
                     uint32_t& duplicateNbEvents,
                     uint32_t& corruptNbEvents,
                     uint32_t& nbBXerrors,
                     uint32_t& nbFedCRCerrors,
                     uint32_t& nbSlinkCRCerrors);

        /**
         * Set maximum rate for generating fragments
         */
        void setMaxTriggerRate(const uint32_t rate);

        /**
         * Create a resync at the given event number
         */
        void resyncAtEvent(const uint32_t rate);


      private:

        void cacheData(const std::string& playbackDataFile);
        bool fillData(toolbox::mem::Reference*&,
                      const uint32_t stopAtEventNumber,
                      uint32_t& lastEventNumber,
                      uint32_t& skipNbEvents,
                      uint32_t& duplicateNbEvents,
                      uint32_t& corruptNbEvents,
                      uint32_t& nbBXerrors,
                      uint32_t& nbFedCRCerrors,
                      uint32_t& nbSlinkCRCerrors);
        toolbox::mem::Reference* clone(toolbox::mem::Reference*) const;

        EvBid evbId_;
        uint32_t frameSize_;
        uint32_t fedSize_;
        uint64_t fedId_;
        uint16_t fedCRC_;
        bool usePlayback_;
        toolbox::mem::Pool* fragmentPool_;
        uint32_t fragmentPoolSize_;

        EvBidFactory evbIdFactory_;
        boost::scoped_ptr<FragmentTracker> fragmentTracker_;
        using PlaybackData = std::vector<toolbox::mem::Reference*>;
        PlaybackData playbackData_;
        PlaybackData::const_reverse_iterator playbackDataPos_;
      };

    } } } //namespace evb::test::dummyFEROL

#endif // _evb_test_dummyFEROL_FragmentGenerator_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
