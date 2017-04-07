#include <iostream>

#include "evb/Constants.h"


int main( int argc, const char* argv[] )
{
  if (argc == 1)
    return -1;

  try
  {
    std::cout << evb::resolveIPaddress(argv[1]) << std::endl;
  }
  catch(const char* msg) {
    std::cerr << msg << std::endl;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
