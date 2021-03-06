#include "evb/Exception.h"
#include "evb/InfoSpaceItems.h"

#include <sstream>


void evb::InfoSpaceItems::add
(
  const std::string& name,
  xdata::Serializable* item,
  Listeners listener
)
{
  items_.push_back(std::make_pair(name,
                                  std::make_pair(item, listener)));
}


void evb::InfoSpaceItems::add(const InfoSpaceItems& other)
{
  const Items otherItems = other.getItems();
  items_.insert(items_.end(), otherItems.begin(), otherItems.end());
}


void evb::InfoSpaceItems::appendItemNames(std::list<std::string>& names) const
{
  for (Items::const_iterator it = items_.begin(), itEnd = items_.end();
       it != itEnd; ++it)
  {
    names.push_back(it->first);
  }
}


void evb::InfoSpaceItems::clear()
{
  // Should they also be removed from the infospace ?
  items_.clear();
}


void evb::InfoSpaceItems::putIntoInfoSpace(xdata::InfoSpace* s, xdata::ActionListener* l) const
{
  for (Items::const_iterator it = items_.begin(), itEnd = items_.end(); it != itEnd; ++it)
  {
    try
    {
      s->fireItemAvailable(it->first, it->second.first);
      switch (it->second.second) {
        case change :
          s->addItemChangedListener(it->first, l);
          break;
        case retrieve :
          s->addItemRetrieveListener(it->first, l);
          break;
        case none :
          break;
      }
    }
    catch(xcept::Exception& e)
    {
      std::ostringstream msg;
      msg << "Failed to put " << it->first;
      msg << " into infospace " << s->name();
      XCEPT_RETHROW(exception::Monitoring, msg.str(), e);
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
