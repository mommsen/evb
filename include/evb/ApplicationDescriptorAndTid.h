#ifndef _evb_ApplicationDescriptorAndTid_h_
#define _evb_ApplicationDescriptorAndTid_h_

#include <set>

#include "i2o/i2oDdmLib.h"
#include "xdaq/ApplicationDescriptor.h"


namespace evb {

  /**
   * Keeps an application's descriptor together with its I2O TID.
   *
   * The main use of this structure is to reduce the number of calls to
   * i2o::AddressMap.
   */
  struct ApplicationDescriptorAndTid
  {
    xdaq::ApplicationDescriptor *descriptor;
    I2O_TID tid;
    
    /**
     * Constructor which initialises both the application descriptor and
     * I2O_TID to 0.
     */
    ApplicationDescriptorAndTid() :
    descriptor(0),tid(0) {}
    
    friend bool operator<(
      const ApplicationDescriptorAndTid& a,
      const ApplicationDescriptorAndTid& b
    )
    {
      if ( a.descriptor != b.descriptor ) return ( a.descriptor < b.descriptor );
      return ( a.tid < b.tid );
    }
  };
  
  typedef std::set<ApplicationDescriptorAndTid> ApplicationDescriptorsAndTids;

} // namespace evb

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
