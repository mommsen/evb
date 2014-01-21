#ifndef _evb_UnsignedInteger32Less_h_
#define _evb_UnsignedInteger32Less_h_

#include "xdata/UnsignedInteger32.h"


namespace evb { // namespace evb

  /**
   * Functor that can be used with the STL in order to determine if one
   * xdata::UnsignedInteger32 is less than another.
   */
  class UnsignedInteger32Less
  {
  public:

    /**
     * Returns true if ul1 is less than ul2.
     */
    bool operator()
    (
      const xdata::UnsignedInteger32 &ul1,
      const xdata::UnsignedInteger32 &ul2
    ) const
    { return ul1.value_ < ul2.value_; }
  };

} // namespace evb

#endif // _evb_UnsignedInteger32Less_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
