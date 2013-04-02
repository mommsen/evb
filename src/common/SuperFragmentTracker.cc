#include <algorithm>
#include <time.h> 

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "evb/Exception.h"
#include "evb/SuperFragmentTracker.h"


evb::SuperFragmentTracker::SuperFragmentTracker
(
  const uint32_t fedPayloadSize,
  const uint32_t fedPayloadStdDev
) :
fedPayloadSize_(fedPayloadSize),
useLogNormal_(fedPayloadStdDev > 0),
logNormalGen_(toolbox::math::LogNormalGen(time(0),fedPayloadSize,fedPayloadStdDev))
{}


void evb::SuperFragmentTracker::startSuperFragment
(
  const FedSourceIds& fedSourceIds
)
{
  fedSourceIds_ = fedSourceIds;
  currentFed_ = fedSourceIds_.begin();
  typeOfNextComponent_ = FED_HEADER;
  remainingFedPayload_ = 0;
}


bool evb::SuperFragmentTracker::reachedEndOfSuperFragment() const
{
  return ( fedSourceIds_.end() == currentFed_ );
}


evb::SuperFragmentTracker::FedComponentDescriptor
evb::SuperFragmentTracker::getNextComponent
(
  const size_t nbBytesAvailable
) const
{
  FedComponentDescriptor component;
  
  if ( reachedEndOfSuperFragment() )
  {
    XCEPT_RAISE(exception::SuperFragment,
      "Reached end of super-fragment");
  }
  
  //////////////////////////////////////
  // Calculate size of next component //
  //////////////////////////////////////
  
  switch (typeOfNextComponent_)
  {
    case FED_HEADER:
      component.size = sizeof(fedh_t);
      break;
    case FED_PAYLOAD:
      if(nbBytesAvailable < 8)
      {
        // Minium payload size is 8 bytes
        component.size = 8;
      }
      else if (remainingFedPayload_ > nbBytesAvailable)
      {
        // Payload must always be a multiple of 8 bytes
        component.size = nbBytesAvailable - (nbBytesAvailable % 8);
      }
      else
      {
        component.size = remainingFedPayload_;
      }
      break;
    case FED_TRAILER:
      component.size = sizeof(fedt_t);
      break;
    default:
      XCEPT_RAISE(exception::SuperFragment, "Unknown component type");
  }
  
  
  //////////////////////////////////////////////////////////
  // Complete and return the description of the component // 
  //////////////////////////////////////////////////////////
  
  component.type = typeOfNextComponent_;
  component.fedId = *currentFed_;
  
  return component;
}


void evb::SuperFragmentTracker::moveToNextComponent
(
  const size_t nbBytesAvailable
)
{
  FedComponentDescriptor currentComponent;
  
  // Next component now becomes current component
  try
  {
    currentComponent = getNextComponent(nbBytesAvailable);
  }
  catch(xcept::Exception e)
  {
    XCEPT_RETHROW(exception::SuperFragment,
      "Failed to get next component", e);
  }
  
  switch(currentComponent.type)
  {
    case FED_HEADER:
      if(fedPayloadSize_ > 0)
      {
        typeOfNextComponent_ = FED_PAYLOAD;
        if ( useLogNormal_ )
          fedPayloadSize_ = std::max(logNormalGen_.getRawRandomSize() & ~0x7,8UL);
        
        remainingFedPayload_ = fedPayloadSize_;
      }
      else
      {
        typeOfNextComponent_ = FED_TRAILER;
        remainingFedPayload_ = 0;
      }
      break;
    case FED_PAYLOAD:
      if(remainingFedPayload_ > currentComponent.size)
      {
        remainingFedPayload_ -= currentComponent.size;
      }
      else
      {
        remainingFedPayload_ = 0;
        typeOfNextComponent_ = FED_TRAILER;
      }
      break;
    case FED_TRAILER:
      typeOfNextComponent_ = FED_HEADER;
      ++currentFed_;
      break;
    default:
      XCEPT_RAISE(exception::SuperFragment, "Unknown component type");
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
