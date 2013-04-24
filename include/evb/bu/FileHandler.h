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
        boost::shared_ptr<StateMachine>,
        const uint32_t buInstance,
        const boost::filesystem::path& rawDataDir,
        const boost::filesystem::path& metaDataDir,
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
       * Increment the number of allocated events
       */
      void incrementAllocatedEventCount()
      { ++allocatedEventCount_; }
      
      /**
       * Increment the number of events allocated to this handler
       * Return true if maximum events reached
       */
      uint32_t getAllocatedEventCount()
      { return allocatedEventCount_; }
      
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
      
      boost::shared_ptr<StateMachine> stateMachine_;
      uint32_t buInstance_;
      const boost::filesystem::path rawDataDir_;
      const boost::filesystem::path metaDataDir_;
      boost::filesystem::path fileName_;
      int fileDescriptor_;
      uint64_t fileSize_;
      uint32_t eventCount_;
      uint32_t allocatedEventCount_;
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
