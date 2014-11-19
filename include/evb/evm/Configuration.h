#ifndef _evb_evm_configuration_h_
#define _evb_evm_configuration_h_

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"
#include "evb/UnsignedInteger32Less.h"
#include "evb/readoutunit/Configuration.h"
#include "xdaq/ApplicationContext.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"


namespace evb {

  namespace evm {

    /**
     * \ingroup xdaqApps
     * \brief Configuration for the readout units (EVM/RU)
     */
    struct Configuration : public readoutunit::Configuration
    {
      xdata::String triggerType;                             // The type of trigger for extracting LS. Valid values are "TCDS","GTP","GTPe","None"
      xdata::UnsignedInteger32 maxTriggerRate;               // Maximum trigger rate in Hz when generating dummy data. 0 means no limitation.
      xdata::Vector<xdata::UnsignedInteger32> ruInstances;   // Vector of RU instances served from the EVM
      xdata::UnsignedInteger32 maxTriggerAgeMSec;            // Maximum time in milliseconds before sending a response to event requests
      xdata::UnsignedInteger32 fakeLumiSectionDuration;      // Duration in seconds of a fake luminosity section. If 0, don't generate lumi sections

      Configuration()
        : triggerType("None"),
          maxTriggerRate(0),
          maxTriggerAgeMSec(1000),
          fakeLumiSectionDuration(0)
      {};

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        const uint32_t instance,
        xdaq::ApplicationContext* context
      )
      {
        readoutunit::Configuration::addToInfoSpace(params,instance,context);
        fillDefaultRUinstances(instance,context);

        params.add("triggerType", &triggerType);
        params.add("maxTriggerRate", &maxTriggerRate, InfoSpaceItems::change);
        params.add("ruInstances", &ruInstances);
        params.add("maxTriggerAgeMSec", &maxTriggerAgeMSec);
        params.add("fakeLumiSectionDuration", &fakeLumiSectionDuration);
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

  } } // namespace evb::evm

#endif // _evb_evm_configuration_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
