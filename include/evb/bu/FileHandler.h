#ifndef _evb_bu_FileHandler_h_
#define _evb_bu_FileHandler_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>


namespace evb {
  namespace bu {

    class StateMachine;

    /**
     * \ingroup xdaqApps
     * \brief Represent an event
     */

    class FileHandler
    {
    public:

      FileHandler
      (
        const uint32_t buInstance,
        const boost::filesystem::path& runRawDataDir,
        const boost::filesystem::path& runMetaDataDir,
        const uint32_t lumiSection,
        const uint32_t index
      );

      ~FileHandler();

      /**
       * Increment the number of events written
       */
      void incrementEventCount()
      { ++eventCount_; }

      /**
       * Get the number of events written
       */
      uint32_t getEventCount() const
      { return eventCount_; }

      /**
       * Return a memory mapped portion of the file with
       * the specified length. The length must be a multiple of
       * the memory page size as returned by sysconf(_SC_PAGE_SIZE).
       */
      void* getMemMap(const size_t length);

      /**
       * Close the file and do the bookkeeping.
       */
      void close();


    private:

      void writeJSON() const;
      void defineJSON(const boost::filesystem::path&) const;
      void calcAdler32(const unsigned char* address, size_t length);

      const uint32_t buInstance_;
      const boost::filesystem::path runRawDataDir_;
      const boost::filesystem::path runMetaDataDir_;
      boost::filesystem::path fileName_;
      int fileDescriptor_;
      uint64_t fileSize_;
      uint32_t eventCount_;
      uint32_t adlerA_;
      uint32_t adlerB_;

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
