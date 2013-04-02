#ifndef _evb_bu_LumiHandler_h_
#define _evb_bu_LumiHandler_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/bu/FileHandler.h"


namespace evb {
  namespace bu {
    
    class StateMachine;
    
    /**
     * \ingroup xdaqApps
     * \brief Write events belonging to the same lumi section
     */
    
    class LumiHandler
    {
    public:
      
      LumiHandler
      (
        const uint32_t buInstance,
        const boost::filesystem::path& rawDataDir,
        const boost::filesystem::path& metaDataDir,
        const uint32_t lumiSection,
        const uint32_t maxEventsPerFile,
        const uint32_t numberOfWriters
      );
      
      ~LumiHandler();
      
      /**
       * Return the next file handler to be used to write one event
       */
      FileHandlerPtr getFileHandler(boost::shared_ptr<StateMachine>);
      
      /**
       * Close the lumi section and do the bookkeeping
       */
      void close();
      
      
    private:
      
      void writeJSON() const;
      void defineJSON(const boost::filesystem::path&) const;
      
      const uint32_t buInstance_;
      const boost::filesystem::path rawDataDir_;
      const boost::filesystem::path metaDataDir_;
      const uint32_t lumiSection_;
      const uint32_t maxEventsPerFile_;
      const uint32_t numberOfWriters_;
      uint32_t index_;
      uint32_t nextFileHandler_;
      uint32_t eventsPerLS_;
      uint32_t filesPerLS_;
      
      typedef std::vector<FileHandlerPtr> FileHandlers;
      FileHandlers fileHandlers_;
      
    }; // LumiHandler

    typedef boost::shared_ptr<LumiHandler> LumiHandlerPtr;
    
  } } // namespace evb::bu

#endif // _evb_bu_LumiHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
