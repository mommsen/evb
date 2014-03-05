#ifndef _evb_bu_FileHandler_h_
#define _evb_bu_FileHandler_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string.h>

#include "evb/bu/FileStatistics.h"


namespace evb {
  namespace bu {

    /**
     * \ingroup xdaqApps
     * \brief Represent an event
     */

    class FileHandler
    {
    public:

      FileHandler(FileStatisticsPtr);

      /**
       * Return a memory mapped portion of the file with
       * the specified length. The length must be a multiple of
       * the memory page size as returned by sysconf(_SC_PAGE_SIZE).
       */
      void* getMemMap(const size_t length);

      /**
       * Close the file and do the bookkeeping.
       */
      FileStatisticsPtr closeAndGetFileStatistics();

      /**
       * Update the file statistics for the given event
       */
      void eventWritten(const uint32_t eventNumber);

      /**
       * Return the number of events written to disk
       */
      uint32_t getNumberOfEventsWritten() const
      { return fileStatistics_->nbEventsWritten; }

      /**
       * Return the time when the file was opened
       */
      uint32_t getCreationTime() const
      { return fileStatistics_->creationTime; }

    private:

      const FileStatisticsPtr fileStatistics_;
      int fileDescriptor_;

    }; // FileHandler

    typedef boost::shared_ptr<FileHandler> FileHandlerPtr;

  } } // namespace evb::bu

#endif // _evb_bu_FileHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
