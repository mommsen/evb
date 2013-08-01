#ifndef _evb_bu_h_
#define _evb_bu_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

#include "evb/EvBApplication.h"
#include "evb/PerformanceMonitor.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/StateMachine.h"
#include "evb/bu/States.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoop.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/Output.h"


namespace evb {

  class InfoSpaceItems;

  namespace bu {
    class EventBuilder;
    class ResourceManager;
    class RUproxy;
  }

  /**
   * \ingroup xdaqApps
   * \brief Builder Unit (BU)
   */

  class BU : public EvBApplication<bu::Configuration,bu::StateMachine>
  {

  public:

    BU(xdaq::ApplicationStub*);

    virtual ~BU() {};

    XDAQ_INSTANTIATOR();


  private:

    virtual void do_bindI2oCallbacks();
    inline void I2O_BU_CACHE_Callback(toolbox::mem::Reference*);

    virtual void do_appendApplicationInfoSpaceItems(InfoSpaceItems&);
    virtual void do_appendMonitoringInfoSpaceItems(InfoSpaceItems&);
    virtual void do_updateMonitoringInfo();

    virtual void do_handleItemRetrieveEvent(const std::string& item);

    virtual void bindNonDefaultXgiCallbacks();
    virtual void do_defaultWebPage(xgi::Output*);

    void superFragmentFIFOWebPage(xgi::Input*, xgi::Output*);
    void freeResourceFIFOWebPage(xgi::Input*, xgi::Output*);
    void blockedResourceFIFOWebPage(xgi::Input*, xgi::Output*);

    boost::shared_ptr<bu::DiskWriter> diskWriter_;
    boost::shared_ptr<bu::ResourceManager> resourceManager_;
    boost::shared_ptr<bu::EventBuilder> eventBuilder_;
    boost::shared_ptr<bu::RUproxy> ruProxy_;

    xdata::UnsignedInteger32 eventSize_;
    xdata::UnsignedInteger32 nbEventsInBU_;
    xdata::UnsignedInteger32 nbEventsBuilt_;
    xdata::UnsignedInteger32 nbEvtsCorrupted_;
    xdata::UnsignedInteger32 nbFilesWritten_;
  };


} //namespace evb

#endif // _evb_bu_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
