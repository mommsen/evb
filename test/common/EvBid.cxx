#include <assert.h>
#include <cstdlib>
#include <stdint.h>
#include <time.h>

#include "evb/EvBid.h"

int main( int argc, const char* argv[] )
{
  srand( time(0) );
  std::cout << std::hex << "0x" << RAND_MAX << std::endl;

  for (int i = 0; i < 10000000; ++i)
  {
    const uint32_t runNumber   = rand();
    const uint32_t eventNumber = rand() & 0x00ffffff;
    const uint32_t resyncCount = rand() & 0x7fffffff;
    const uint32_t lumiSection = rand() & 0x0fffffff;
    const uint32_t bxId        = rand() & 0x00000fff;
    const bool resynced = (bxId & 0x1) == 1;

    evb::EvBid evbId(resynced, resyncCount,eventNumber,bxId,lumiSection,runNumber);

    assert( evbId.resyncCount() == resyncCount );
    assert( evbId.eventNumber() == eventNumber );
    assert( evbId.bxId() == bxId );
    assert( evbId.lumiSection() == lumiSection );
    assert( evbId.runNumber() == runNumber );
    assert( evbId.resynced() == resynced );
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
