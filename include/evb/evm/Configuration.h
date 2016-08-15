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
      xdata::UnsignedInteger32 maxTriggerRate;               // Maximum trigger rate in Hz when generating dummy data. 0 means no limitation.
      xdata::Vector<xdata::UnsignedInteger32> ruInstances;   // Vector of RU instances served from the EVM
      xdata::UnsignedInteger32 maxTriggerAgeMSec;            // Maximum time in milliseconds before sending a response to event requests
      xdata::Boolean getLumiSectionFromTrigger;              // If set to true, try to get the lumi section number from the trigger. Otherwise, use fake LS
      xdata::UnsignedInteger32 fakeLumiSectionDuration;      // Duration in seconds of a fake luminosity section. If 0, don't generate lumi sections
      xdata::UnsignedInteger32 allocateFIFOCapacity;         // Capacity of the FIFO to store allocation messages
      xdata::UnsignedInteger32 allocateBlockSize;            // I2O block size used for packing readout msg from EVM to RUs
      xdata::UnsignedInteger32 maxAllocateTime;              // Maximum time in ns waited while packing readout msg from EVM to RUs

      Configuration()
        : maxTriggerRate(0),
          maxTriggerAgeMSec(1000),
          getLumiSectionFromTrigger(true),
          fakeLumiSectionDuration(0),
          allocateFIFOCapacity(1440),
          allocateBlockSize(8192),
          maxAllocateTime(100)
      {};

      void addToInfoSpace
      (
        InfoSpaceItems& params,
        xdaq::ApplicationContext* context
      )
      {
        readoutunit::Configuration::addToInfoSpace(params,context);
        fillDefaultRUinstances(context);

        params.add("maxTriggerRate", &maxTriggerRate, InfoSpaceItems::change);
        params.add("ruInstances", &ruInstances);
        params.add("maxTriggerAgeMSec", &maxTriggerAgeMSec);
        params.add("getLumiSectionFromTrigger", &getLumiSectionFromTrigger);
        params.add("fakeLumiSectionDuration", &fakeLumiSectionDuration);
        params.add("allocateFIFOCapacity", &allocateFIFOCapacity);
        params.add("allocateBlockSize", &allocateBlockSize);
        params.add("maxAllocateTime", &maxAllocateTime);
      }

      void fillDefaultRUinstances(xdaq::ApplicationContext* context)
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
