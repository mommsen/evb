#ifndef _evb_bu_RUbroadcaster_h_
#define _evb_bu_RUbroadcaster_h_

#include <boost/thread/mutex.hpp>

#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "evb/ApplicationDescriptorAndTid.h"
#include "evb/InfoSpaceItems.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"



namespace evb { namespace bu { // namespace evb::bu

  /**
   * \ingroup xdaqApps
   * \brief Send I2O messages to multiple RUs
   */
  
  class RUbroadcaster
  {
    
  public:

    RUbroadcaster
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    virtual ~RUbroadcaster() {};
    
    /**
     * Init the RU instances and append necessary
     * configuration parameters to the InfoSpaceItems
     */
    void initRuInstances(InfoSpaceItems&);
    
    /**
     * Find the application descriptors of the participating RUs.
     *
     * By default, all RUs in the zone will participate.
     * This can be overwritten by setting  the instances
     * in 'ruInstances'.
     */
    void getApplicationDescriptors();

    /**
     * Send the data contained in the reference to all RUs.
     */
    void sendToAllRUs
    (
      toolbox::mem::Reference*,
      const size_t bufSize
    );
    
    /**
     * Return the tids of RUs participating in the event building
     */
    std::vector<I2O_TID>& getRuTids()
    { return ruTids_; }

    
  protected:

    xdaq::Application* app_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    I2O_TID tid_;
    
    typedef std::set<ApplicationDescriptorAndTid>
    RUDescriptorsAndTids;
    RUDescriptorsAndTids participatingRUs_;
    
  private:
    
    void getRuInstances();
    void fillParticipatingRUsUsingRuInstances();
    void fillRUInstance(xdata::UnsignedInteger32);

    std::vector<I2O_TID> ruTids_;
    typedef xdata::Vector<xdata::UnsignedInteger32> RUInstances;
    RUInstances ruInstances_;
    boost::mutex ruInstancesMutex_;
    
  };
  
  
} } //namespace evb::bu

#endif // _evb_bu_RUbroadcaster_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
