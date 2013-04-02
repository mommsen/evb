#ifndef _evb_SuperFragmentGenerator_h_
#define _evb_SuperFragmentGenerator_h_

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include "evb/SuperFragmentTracker.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/net/URN.h"
#include "xdata/Vector.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class EvBid;
  class EvBidFactory;
  class L1Information;

  /**
   * \ingroup xdaqApps
   * \brief Provide dummy FED data
   */
    
  class SuperFragmentGenerator
  {
    
  public:

    SuperFragmentGenerator(const std::string& poolName);
    
    ~SuperFragmentGenerator() {};

    /**
     * Configure the super-fragment generator.
     * The fedSourceIds vector specifies the FEDs makeing up a super-fragment
     * If usePlayback is set to true, the data is read from the playbackDataFile,
     * otherwise, dummy data is generated according to the dummyFedPayloadSize.
     * The dummyBlockSize specifies the size of the data blocks.
     */
    void configure
    (
      const xdata::Vector<xdata::UnsignedInteger32>& fedSourceIds,
      const bool usePlayback,
      const std::string& playbackDataFile,
      const uint32_t dummyBlockSize,
      const uint32_t dummyFedPayloadSize,
      const uint32_t dummyFedPayloadStdDev,
      const uint32_t maxFragmentsInMemory = 0
    );

    /**
     * Start serving events for a new run
     */
    void reset();

    /**
     * Get the next super-fragment.
     * Returns false if no super-fragment can be generated
     */
    bool getData(toolbox::mem::Reference*&);

    /**
     * Get a super-fragment for the specified event number.
     * Returns false if no super-fragment for this event is available
     */
    bool getData
    (
      toolbox::mem::Reference*&,
      const EvBid&
    );

    /**
     * Get a L1 trigger fragment with the specified properties.
     * In case of using a playback file, this call is identical to the above one.
     * Returns false if no super-fragment for this event is available
     */
    bool getData
    (
      toolbox::mem::Reference*&,
      const EvBid&,
      const L1Information&
    );
    
    /**
     * Return the memory pool usage (in Bytes)
     */
    size_t getMemoryUsage() const;

    
  private:

    void cacheData(const std::string& playbackDataFile);
    bool getFragmentFromPlayback(toolbox::mem::Reference*&, const EvBid&);
    bool getSuperFragment
    (
      toolbox::mem::Reference*&,
      const EvBid&
    );
    void fillBlock
    (
      toolbox::mem::Reference*,
      uint16_t blockNb,
      const EvBid&
    );
    void insertFedComponent
    (
      const SuperFragmentTracker::FedComponentDescriptor&,
      const unsigned char* startAddr,
      const uint32_t eventNumber
    );
    void fillRuAndFrlHeaders
    (
      const unsigned char* blockAddr,
      const size_t i2oMessageSize,
      const size_t nbFedBytesWritten,
      const EvBid&,
      const uint16_t blockNb,
      const bool isLastBlockOfSuperFragment
    ) const;
    void setNbBlocksInSuperFragment
    (
      toolbox::mem::Reference*,
      const uint32_t nbBlocks
    ) const;
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
    toolbox::mem::Reference* clone(toolbox::mem::Reference*) const;
    
    toolbox::mem::Pool* dummySuperFragmentPool_;
    SuperFragmentTracker::FedSourceIds fedSourceIds_;
    EvBidFactory evbIdFactory_;
    uint32_t dummyBlockSize_;
    uint32_t dummyFedPayloadSize_;
    uint32_t eventNumber_;
    uint16_t fedCRC_;
    bool usePlayback_;

    boost::scoped_ptr<SuperFragmentTracker> superFragmentTracker_;
    
    typedef std::map<EvBid,toolbox::mem::Reference*> PlaybackData;
    PlaybackData playbackData_;
    PlaybackData::const_iterator playbackDataPos_;

  };
  
} //namespace evb

#endif // _evb_SuperFragmentGenerator_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
