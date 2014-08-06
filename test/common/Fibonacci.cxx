#include <assert.h>
#include <stdint.h>

#include "evb/Constants.h"

int main( int argc, const char* argv[] )
{
  uint32_t value = 1;
  assert( evb::isFibonacci(value) );

  value = 6;
  assert( ! evb::isFibonacci(value) );

  value = 102334155;
  assert( evb::isFibonacci(value) );

  value = 102334156;
  assert( ! evb::isFibonacci(value) );

  value = 102337316;
  assert( ! evb::isFibonacci(value) );

  uint64_t valueLong = 12200160415121876738ULL;
  assert( evb::isFibonacci(valueLong) );

  valueLong = 12200160415121876737ULL;
  assert( ! evb::isFibonacci(valueLong) );

  valueLong = 12200160415121876739ULL;
  assert( ! evb::isFibonacci(valueLong) );
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
