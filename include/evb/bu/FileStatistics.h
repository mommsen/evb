#ifndef _evb_bu_FileStatistics_h_
#define _evb_bu_FileStatistics_h_

#include <memory>
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
    using FileStatisticsPtr = std::shared_ptr<FileStatistics>;


    inline std::ostream& operator<<
    (
      std::ostream& s,
      const evb::bu::FileStatisticsPtr fileStatistics
    )
    {
      if ( fileStatistics.get() )
      {
        s << "creationTime=" << asctime(gmtime(&fileStatistics->creationTime)) << " GMT ";
        s << "lumiSection=" << fileStatistics->lumiSection << " ";
        s << "fileName=" << fileStatistics->fileName << " ";
        s << "nbEventsWritten=" << fileStatistics->nbEventsWritten << " ";
        s << "lastEventNumberWritten=" << fileStatistics->lastEventNumberWritten << " ";
        s << "fileSize=" << fileStatistics->fileSize << " Bytes";
      }

      return s;
    }

  } } // namespace evb::bu

#endif // _evb_bu_FileStatistics_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
