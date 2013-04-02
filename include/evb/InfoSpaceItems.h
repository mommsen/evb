#ifndef _evb_InfoSpaceItems_h_
#define _evb_InfoSpaceItems_h_

#include "xdata/InfoSpace.h"
#include "xdata/Serializable.h"
#include "xgi/Output.h"

#include <utility>
#include <vector>
#include <string>


namespace evb {

  class InfoSpaceItems
  {

  public:

    enum Listeners { none, change, retrieve };

    /**
     * Add the passed parameters to the items
     */
    void add(const std::string& name, xdata::Serializable* value, Listeners listener = none);

    /**
     * Add the passed parameters to the items
     */
    void add(const InfoSpaceItems&);

    /**
     * Add the names of the stored items to the list
     */
    void appendItemNames(std::list<std::string>&) const;

    /**
     * Remove any parameters
     */
    void clear();

    /**
     * Puts the parameters into the specified info space.
     */
    void putIntoInfoSpace(xdata::InfoSpace*, xdata::ActionListener*) const;

    /**
     * Print the parameters as HTML snipped
     */
    void printHtml(const std::string& title, xgi::Output*) const;

  private:

    typedef std::vector< std::pair<std::string, std::pair<xdata::Serializable*,Listeners> > > Items;
    inline Items getItems() const { return items_; }

    Items items_;

  };

} // namespace evb

#endif // _evb_InfoSpaceItems_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
