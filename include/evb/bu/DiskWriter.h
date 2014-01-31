#ifndef _evb_bu_DiskWriter_h_
#define _evb_bu_DiskWriter_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <curl/curl.h>
#include <map>
#include <stdint.h>

#include "evb/bu/Configuration.h"
#include "evb/bu/StreamHandler.h"
#include "evb/InfoSpaceItems.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WorkLoop.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class BU;

  namespace bu { // namespace evb::bu

    class RUproxy;
    class ResourceManager;
    class StateMachine;

    /**
     * \ingroup xdaqApps
     * \brief Write events to disk
     */

    class DiskWriter : public toolbox::lang::Class
    {
    public:

      DiskWriter
      (
        BU*,
        boost::shared_ptr<ResourceManager>
      );

      /**
       * Get a stream handler for the given builder id
       */
      StreamHandlerPtr getStreamHandler(const uint16_t builderId) const;

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
       * Configure
       */
      void configure();

      /**
       * Register the RU proxy
       */
      void registerRUproxy(boost::shared_ptr<RUproxy> ruProxy)
      { ruProxy_ = ruProxy; }

      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }

      /**
       * Start processing messages
       */
      void startProcessing(const uint32_t runNumber);

      /**
       * Drain messages
       */
      void drain();

      /**
       * Stop processing messages
       */
      void stopProcessing();

      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*) const;


    private:

      struct LumiInfo
      {
        const uint32_t lumiSection;
        uint32_t totalEvents;
        uint32_t nbEvents;
        uint32_t nbEventsWritten;
        uint32_t fileCount;
        uint32_t index;

        LumiInfo(const uint32_t ls)
        : lumiSection(ls),totalEvents(0),nbEvents(0),nbEventsWritten(0),fileCount(0),index(0) {};
      };
      typedef boost::shared_ptr<LumiInfo> LumiInfoPtr;
      typedef std::map<uint32_t,LumiInfoPtr> LumiStatistics;
      LumiStatistics lumiStatistics_;
      boost::mutex lumiStatisticsMutex_;

      void resetMonitoringCounters();
      void startLumiAccounting();
      void startFileMover();
      bool lumiAccounting(toolbox::task::WorkLoop*);
      bool fileMover(toolbox::task::WorkLoop*);
      void doLumiSectionAccounting();
      void moveFiles();
      void handleRawDataFile(const StreamHandler::FileStatisticsPtr&);
      LumiStatistics::iterator getLumiStatistics(const uint32_t lumiSection);
      void createDir(const boost::filesystem::path&) const;
      void removeDir(const boost::filesystem::path&) const;
      void closeAnyOldRuns() const;
      void getHLTmenu(const boost::filesystem::path& runDir) const;
      void retrieveFromURL(CURL*, const std::string& url, const boost::filesystem::path& output) const;
      void createLockFile(const boost::filesystem::path&) const;
      void writeEoLS(const LumiInfoPtr&) const;
      void writeEoR() const;
      void defineRawData(const boost::filesystem::path& jsdDir);
      void defineEoLS(const boost::filesystem::path& jsdDir);
      void defineEoR(const boost::filesystem::path& jsdDir);

      BU* bu_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;
      const ConfigurationPtr configuration_;

      const uint32_t buInstance_;
      uint32_t runNumber_;

      boost::filesystem::path runRawDataDir_;
      boost::filesystem::path runMetaDataDir_;
      boost::filesystem::path rawDataDefFile_;
      boost::filesystem::path eolsDefFile_;
      boost::filesystem::path eorDefFile_;

      typedef std::map<uint16_t,StreamHandlerPtr > StreamHandlers;
      StreamHandlers streamHandlers_;

      toolbox::task::WorkLoop* lumiAccountingWorkLoop_;
      toolbox::task::WorkLoop* fileMoverWorkLoop_;
      toolbox::task::ActionSignature* lumiAccountingAction_;
      toolbox::task::ActionSignature* fileMoverAction_;
      bool doProcessing_;
      bool lumiAccountingActive_;
      bool fileMoverActive_;

      struct DiskWriterMonitoring
      {
        uint32_t nbFiles;
        uint32_t nbEventsWritten;
        uint32_t nbLumiSections;
        uint32_t lastEventNumberWritten;
        uint32_t currentLumiSection;
      } diskWriterMonitoring_;
      mutable boost::mutex diskWriterMonitoringMutex_;

      xdata::UnsignedInteger32 nbFilesWritten_;
      xdata::UnsignedInteger32 nbLumiSections_;

    };

  } } // namespace evb::bu


#endif // _evb_bu_DiskWriter_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
