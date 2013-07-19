#ifndef _evb_FragmentGenerator_h_
#define _evb_FragmentGenerator_h_

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

  class L1Information;

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
      const uint32_t fedSizeStdDev,
      const size_t fragmentPoolSize
    );

    /**
     * Start serving events for a new run
     */
    void reset();

    /**
     * Get the next fragment.
     * Returns false if no fragment can be generated
     */
    bool getData(toolbox::mem::Reference*&);


  private:

    void cacheData(const std::string& playbackDataFile);
    bool fillData(toolbox::mem::Reference*&);
    toolbox::mem::Reference* clone(toolbox::mem::Reference*) const;
    void fillTriggerPayload
    (
      unsigned char* fedPtr,
      const uint32_t eventNumber,
      const L1Information&
    ) const;
    void updateCRC
    (
      const unsigned char* fedPtr
    ) const;

    uint32_t frameSize_;
    uint32_t fedSize_;
    uint32_t eventNumber_;
    uint64_t fedId_;
    uint16_t fedCRC_;
    bool usePlayback_;
    toolbox::mem::Pool* fragmentPool_;
    uint32_t fragmentPoolSize_;

    boost::scoped_ptr<FragmentTracker> fragmentTracker_;
    typedef std::vector<toolbox::mem::Reference*> PlaybackData;
    PlaybackData playbackData_;
    PlaybackData::const_iterator playbackDataPos_;

  };

} //namespace evb

#endif // _evb_FragmentGenerator_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
