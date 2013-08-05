#ifndef _evb_bu_configuration_h_
#define _evb_bu_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/InfoSpaceItems.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/Integer32.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  namespace bu {

    /**
    * \ingroup xdaqApps
    * \brief Configuration for the builder unit (BU)
    */
    struct Configuration
    {
      xdata::Integer32 evmInstance;
      xdata::UnsignedInteger32 maxEvtsUnderConstruction;
      xdata::UnsignedInteger32 eventsPerRequest;
      xdata::UnsignedInteger32 superFragmentFIFOCapacity;
      xdata::Boolean dropEventData;
      xdata::UnsignedInteger32 numberOfBuilders;
      xdata::String rawDataDir;
      xdata::String metaDataDir;
      xdata::Double rawDataHighWaterMark;
      xdata::Double rawDataLowWaterMark;
      xdata::Double metaDataHighWaterMark;
      xdata::Double metaDataLowWaterMark;
      xdata::UnsignedInteger32 maxEventsPerFile;
      xdata::UnsignedInteger32 lumiMonitorFIFOCapacity;
      xdata::UnsignedInteger32 lumiSectionTimeout;

      Configuration()
      : evmInstance(-1), // Explicitly indicate parameter not set
        maxEvtsUnderConstruction(64),
        eventsPerRequest(8),
        superFragmentFIFOCapacity(16384),
        dropEventData(false),
        numberOfBuilders(3),
        rawDataDir("/tmp/fff"),
        metaDataDir("/tmp/fff"),
        rawDataHighWaterMark(0.7),
        rawDataLowWaterMark(0.5),
        metaDataHighWaterMark(0.9),
        metaDataLowWaterMark(0.5),
        maxEventsPerFile(2000),
        lumiMonitorFIFOCapacity(128),
        lumiSectionTimeout(25)
      {};

      void addToInfoSpace(InfoSpaceItems& params, const uint32_t instance)
      {
        params.add("evmInstance", &evmInstance);
        params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction);
        params.add("eventsPerRequest", &eventsPerRequest);
        params.add("superFragmentFIFOCapacity", &superFragmentFIFOCapacity);
        params.add("dropEventData", &dropEventData);
        params.add("numberOfBuilders", &numberOfBuilders);
        params.add("rawDataDir", &rawDataDir);
        params.add("metaDataDir", &metaDataDir);
        params.add("rawDataHighWaterMark", &rawDataHighWaterMark);
        params.add("rawDataLowWaterMark", &rawDataLowWaterMark);
        params.add("metaDataHighWaterMark", &metaDataHighWaterMark);
        params.add("metaDataLowWaterMark", &metaDataLowWaterMark);
        params.add("maxEventsPerFile", &maxEventsPerFile);
        params.add("lumiMonitorFIFOCapacity", &lumiMonitorFIFOCapacity);
        params.add("lumiSectionTimeout", &lumiSectionTimeout);
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
