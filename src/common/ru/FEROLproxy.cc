#include <algorithm>
#include <byteswap.h>
#include <iterator>
#include <string.h>

#include "interface/shared/fed_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "evb/ru/InputHandler.h"
#include "evb/Constants.h"
#include "evb/Exception.h"
#include "toolbox/net/URN.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"


evb::ru::FEROLproxy::FEROLproxy()
{}


void evb::ru::FEROLproxy::dataReadyCallback(toolbox::mem::Reference* bufRef)
{
  //addFragment(bufRef);
  bufRef->release();
}


bool evb::ru::FEROLproxy::getNextAvailableSuperFragment(SuperFragmentPtr)
{
  return false;
}


bool evb::ru::FEROLproxy::getSuperFragmentWithEvBid(const EvBid&, SuperFragmentPtr)
{
  return false;
}


void evb::ru::FEROLproxy::clear()
{
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
