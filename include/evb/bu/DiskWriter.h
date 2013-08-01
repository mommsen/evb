#ifndef _evb_bu_DiskWriter_h_
#define _evb_bu_DiskWriter_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/Configuration.h"
#include "evb/bu/StreamHandler.h"
#include "evb/InfoSpaceItems.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class BU;

  namespace bu { // namespace evb::bu

    /**
     * \ingroup xdaqApps
     * \brief Write events to disk
     */

    class DiskWriter
    {
    public:

      DiskWriter(BU*);

      /**
       * Get a stream handler for the given builder id
       */
      StreamHandlerPtr getStreamHandler(const uint16_t builderId);

      /**
       * Append the info space items to be published in the
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);

      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();

      /**
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();

      /**
       * Configure
       */
      void configure();

      /**
       * Remove all data
       */
      void clear();

      /**
       * Start processing messages
       */
      void startProcessing(const uint32_t runNumber);

      /**
       * Stop processing messages
       */
      void stopProcessing();

      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);

      /**
       * Create the directory if it does not exist
       */
      static void createDir(const boost::filesystem::path&);

      /**
       * Remove the directory if it exists
       */
      static void removeDir(const boost::filesystem::path&);


    private:

      void writeJSON();
      void defineJSON(const boost::filesystem::path&) const;

      BU* bu_;
      const ConfigurationPtr configuration_;

      const uint32_t buInstance_;
      uint32_t runNumber_;

      boost::filesystem::path buRawDataDir_;
      boost::filesystem::path buMetaDataDir_;
      boost::filesystem::path runRawDataDir_;
      boost::filesystem::path runMetaDataDir_;

      typedef std::map<uint16_t,StreamHandlerPtr > StreamHandlers;
      StreamHandlers streamHandlers_;

      struct DiskWriterMonitoring
      {
        uint32_t nbFiles;
        uint32_t nbEventsWritten;
        uint32_t nbLumiSections;
        uint32_t lastEventNumberWritten;
        uint32_t currentLumiSection;
        uint32_t lastEoLS;
      } diskWriterMonitoring_;
      boost::mutex diskWriterMonitoringMutex_;

      xdata::UnsignedInteger32 nbEvtsWritten_;
      xdata::UnsignedInteger32 nbFilesWritten_;

    };

  } } // namespace evb::bu

#endif // _evb_bu_DiskWriter_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
