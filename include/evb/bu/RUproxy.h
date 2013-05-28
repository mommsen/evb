#ifndef _evb_bu_RUproxy_h_
#define _evb_bu_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "evb/EvBid.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/bu/RUbroadcaster.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace evb { namespace bu { // namespace evb::bu

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-BU communication
   */
  
  class RUproxy : public RUbroadcaster
  {
    
  public:

    RUproxy
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    virtual ~RUproxy() {};
    
    /**
     * Callback for I2O message containing a super fragment
     */
    void superFragmentCallback(toolbox::mem::Reference*);
    
    /**
     * Fill the next available data block
     * into the passed buffer reference.
     * Return false if no data is available
     */
    bool getData(toolbox::mem::Reference*&);

    /**
     * Send request for N trigger data fragments to the RUs
     */
    void requestTriggerData(const uint32_t buResourceId, const uint32_t count);

    /**
     * Send request for event fragments for the given
     * event-builder ids to the RUS
     */
    void requestFragments(const uint32_t buResourceId, const std::vector<EvBid>&);

    /**
     * Append the info space parameters used for the
     * configuration to the InfoSpaceItems
     */
    void appendConfigurationItems(InfoSpaceItems&);
    
    /**
     * Append the info space items to be published in the 
     * monitoring info space to the InfoSpaceItems
     */
    void appendMonitoringItems(InfoSpaceItems&);
    
    /**
     * Update all values of the items put into the monitoring
     * info space. The caller has to make sure that the info
     * space where the items reside is locked and properly unlocked
     * after the call.
     */
    void updateMonitoringItems();
    
    /**
     * Reset the monitoring counters
     */
    void resetMonitoringCounters();
   
    /**
     * Configure
     */
    void configure();

    /**
     * Remove all data
     */
    void clear();
  
    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the data block FIFO as HTML snipped
     */
    inline void printBlockFIFO(xgi::Output* out)
    { blockFIFO_.printVerticalHtml(out); }
    
    
  private:
    
    void updateBlockCounters(toolbox::mem::Reference*);

    typedef OneToOneQueue<toolbox::mem::Reference*> BlockFIFO;
    BlockFIFO blockFIFO_;
    
    typedef std::map<uint32_t,uint64_t> CountsPerRU;
    struct BlockMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumberFromRUs;
      CountsPerRU logicalCountPerRU;
      CountsPerRU payloadPerRU;
    } blockMonitoring_;
    boost::mutex blockMonitoringMutex_;

    struct TriggerRequestMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
    } triggerRequestMonitoring_;
    boost::mutex triggerRequestMonitoringMutex_;

    struct FragmentRequestMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
    } fragmentRequestMonitoring_;
    boost::mutex fragmentRequestMonitoringMutex_;
    
    InfoSpaceItems ruParams_;
    xdata::UnsignedInteger32 blockFIFOCapacity_;

    xdata::UnsignedInteger32 lastEventNumberFromRUs_;
    xdata::UnsignedInteger64 i2oBUCacheCount_;
    xdata::UnsignedInteger64 i2oRUSendCount_;
  };
  
  
} } //namespace evb::bu

#endif // _evb_bu_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
