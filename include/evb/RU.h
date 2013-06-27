#ifndef _evb_ru_h_
#define _evb_ru_h_

#include "evb/readoutunit/Configuration.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "xdaq/ApplicationStub.h"


namespace evb {
  
  namespace ru {
    class RUinput;
    typedef readoutunit::ReadoutUnit<readoutunit::Configuration,RUinput> ReadoutUnit;
  }
  
  /**
   * \ingroup xdaqApps
   * \brief Readout unit (RU)
   */
  class RU : public ru::ReadoutUnit
  {
  public:
    
    RU(xdaq::ApplicationStub*);
    
    XDAQ_INSTANTIATOR();
    
    
  private:
    
  }; // class RU
  
} // namespace evb

#endif // _evb_ru_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
