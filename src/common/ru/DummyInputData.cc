#include "evb/ru/InputHandler.h"
#include "evb/Exception.h"


evb::ru::DummyInputData::DummyInputData()
{}


void evb::ru::DummyInputData::dataReadyCallback(toolbox::mem::Reference* bufRef)
{
  XCEPT_RAISE(exception::Configuration,
    "Received an event fragment while generating dummy data");
}


bool evb::ru::DummyInputData::getNextAvailableSuperFragment(SuperFragmentPtr)
{
  return false;
}


bool evb::ru::DummyInputData::getSuperFragmentWithEvBid(const EvBid&, SuperFragmentPtr)
{
  return false;
}


void evb::ru::DummyInputData::clear()
{
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
