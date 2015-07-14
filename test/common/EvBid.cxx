#include <assert.h>
#include <cstdlib>
#include <stdint.h>
#include <time.h>

#include "evb/EvBid.h"

int main( int argc, const char* argv[] )
{
  srand( time(0) );

  for (int i = 0; i < 100000; ++i)
  {
    uint32_t runNumber = rand();
    uint32_t eventNumber = rand() & 0xffffff;
    uint32_t resyncCount = rand();
    uint32_t lumiSection = rand() & 0xfffffff;
    uint32_t bxId = rand() & 0xfff;

    evb::EvBid evbId(resyncCount,eventNumber,bxId,lumiSection,runNumber);

    assert( evbId.resyncCount() == resyncCount );
    assert( evbId.eventNumber() == eventNumber );
    assert( evbId.bxId() == bxId );
    assert( evbId.lumiSection() == lumiSection );
    assert( evbId.runNumber() == runNumber );
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
