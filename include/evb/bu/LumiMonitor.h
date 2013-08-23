#ifndef _evb_bu_LumiMonitor_h_
#define _evb_bu_LumiMonitor_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <time.h>

namespace evb {

  namespace bu { // namespace evb::bu

    struct LumiMonitor
    {
      const uint32_t lumiSection;
      uint32_t nbFiles;
      uint32_t nbEventsWritten;
      uint32_t lastEventNumberWritten;
      uint32_t updates;
      time_t timeOfLastEvent;

      LumiMonitor(uint32_t ls) :
      lumiSection(ls),nbFiles(0),nbEventsWritten(0),updates(1),
      timeOfLastEvent( time(0) ) {};

      void update(uint32_t eventNumber)
      {
        ++nbEventsWritten;
        lastEventNumberWritten = eventNumber;
        timeOfLastEvent = time(0);
      }

      inline LumiMonitor& operator+= (const LumiMonitor& other)
      {
        this->nbFiles += other.nbFiles;
        this->nbEventsWritten += other.nbEventsWritten;
        this->updates += other.updates;
        if ( this->lastEventNumberWritten < other.lastEventNumberWritten )
          this->lastEventNumberWritten = other.lastEventNumberWritten;
        if ( this->timeOfLastEvent < other.timeOfLastEvent )
          this->timeOfLastEvent = other.timeOfLastEvent;
        return *this;
      }
    };

    typedef boost::shared_ptr<LumiMonitor> LumiMonitorPtr;

    inline bool operator< (const LumiMonitorPtr& lhs, const LumiMonitorPtr& rhs)
    {
      return ( lhs->lumiSection < rhs->lumiSection );
    }

  } } // namespace evb::bu


#endif // _evb_bu_LumiMonitor_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
