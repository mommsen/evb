#ifndef _evb_bu_FileStatistics_h_
#define _evb_bu_FileStatistics_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string.h>
#include <time.h>

namespace evb {

  namespace bu { // namespace evb::bu

    struct FileStatistics
    {
      const time_t creationTime;
      const uint32_t lumiSection;
      const std::string fileName;
      uint32_t nbEventsWritten;
      uint32_t lastEventNumberWritten;
      uint64_t fileSize;

      FileStatistics(const uint32_t ls, const std::string& fileName)
      : creationTime(time(0)),lumiSection(ls),fileName(fileName),nbEventsWritten(0),lastEventNumberWritten(0),fileSize(0) {};
    };
    typedef boost::shared_ptr<FileStatistics> FileStatisticsPtr;

  } } // namespace evb::bu

#endif // _evb_bu_FileStatistics_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
