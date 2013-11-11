#ifndef _evb_bu_FileHandler_h_
#define _evb_bu_FileHandler_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string.h>


namespace evb {
  namespace bu {

    /**
     * \ingroup xdaqApps
     * \brief Represent an event
     */

    class FileHandler
    {
    public:

      FileHandler(const std::string& rawFileName);

      ~FileHandler();

      /**
       * Return a memory mapped portion of the file with
       * the specified length. The length must be a multiple of
       * the memory page size as returned by sysconf(_SC_PAGE_SIZE).
       */
      void* getMemMap(const size_t length);

      /**
       * Close the file and do the bookkeeping.
       */
      uint64_t closeAndGetFileSize();

    private:

      const std::string rawFileName_;
      int fileDescriptor_;

      uint64_t fileSize_;

      boost::mutex mutex_;

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
