#ifndef _evb_bu_DiskWriter_h_
#define _evb_bu_DiskWriter_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "evb/bu/Configuration.h"
#include "evb/bu/DiskUsage.h"
#include "evb/bu/Event.h"
#include "evb/bu/EventBuilder.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/LumiHandler.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {

  class BU;

  namespace bu { // namespace evb::bu

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

      ~DiskWriter();

      /**
       * Write the event given as argument to disk
       */
      void writeEvent(const EventPtr);

      /**
       * Close the given lumi section
       */
      void closeLS(const uint32_t lumiSection);

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
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }

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
       * Print the content of the event FIFO as HTML snipped
       */
      inline void printEventFIFO(xgi::Output* out)
      { eventFIFO_.printVerticalHtml(out); }

      /**
       * Print the content of the EoLS FIFO as HTML snipped
       */
      inline void printEoLSFIFO(xgi::Output* out)
      { eolsFIFO_.printVerticalHtml(out); }


    private:

      void startProcessingWorkLoop();
      bool process(toolbox::task::WorkLoop*);
      bool handleEvents();
      bool handleEoLS();
      LumiHandlerPtr getLumiHandler(const uint32_t lumiSection);
      bool writing(toolbox::task::WorkLoop*);
      bool resourceMonitoring(toolbox::task::WorkLoop*);
      bool checkDiskSize(DiskUsagePtr);
      void writeJSON();
      void defineJSON(const boost::filesystem::path&) const;
      void createWritingWorkLoops();

      BU* bu_;
      boost::shared_ptr<ResourceManager> resourceManager_;
      boost::shared_ptr<StateMachine> stateMachine_;

      const ConfigurationPtr configuration_;

      const uint32_t buInstance_;
      uint32_t runNumber_;
      uint32_t index_;

      boost::filesystem::path buRawDataDir_;
      boost::filesystem::path runRawDataDir_;
      boost::filesystem::path buMetaDataDir_;
      boost::filesystem::path runMetaDataDir_;
      DiskUsagePtr rawDataDiskUsage_;
      DiskUsagePtr metaDataDiskUsage_;

      typedef std::map<uint32_t,LumiHandlerPtr > LumiHandlers;
      LumiHandlers lumiHandlers_;
      boost::mutex lumiHandlersMutex_;

      toolbox::task::WorkLoop* processingWL_;
      toolbox::task::ActionSignature* processingAction_;
      toolbox::task::WorkLoop* resourceMonitoringWL_;
      toolbox::task::ActionSignature* resourceMonitoringAction_;

      typedef std::vector<toolbox::task::WorkLoop*> WorkLoops;
      WorkLoops writingWorkLoops_;
      toolbox::task::ActionSignature* writingAction_;

      typedef OneToOneQueue< EventPtr > EventFIFO;
      EventFIFO eventFIFO_;
      typedef OneToOneQueue<uint32_t> EoLSFIFO;
      EoLSFIFO eolsFIFO_;

      struct FileHandlerAndEvent {
        const FileHandlerPtr fileHandler;
        const EventPtr event;

        FileHandlerAndEvent(const FileHandlerPtr fileHandler, const EventPtr event)
        : fileHandler(fileHandler), event(event) {};
      };
      typedef boost::shared_ptr<FileHandlerAndEvent> FileHandlerAndEventPtr;
      typedef OneToOneQueue<FileHandlerAndEventPtr> FileHandlerAndEventFIFO;
      FileHandlerAndEventFIFO fileHandlerAndEventFIFO_;
      boost::mutex fileHandlerAndEventFIFOmutex_;

      volatile bool writingActive_;
      volatile bool doProcessing_;
      volatile bool processActive_;

      struct DiskWriterMonitoring
      {
        uint32_t nbFiles;
        uint32_t nbEventsWritten;
        uint32_t nbLumiSections;
        uint32_t lastEventNumberWritten;
        uint32_t currentLumiSection;
        uint32_t lastEoLS;
        uint32_t nbEventsCorrupted;
      } diskWriterMonitoring_;
      boost::mutex diskWriterMonitoringMutex_;

      xdata::UnsignedInteger32 nbEvtsWritten_;
      xdata::UnsignedInteger32 nbFilesWritten_;
      xdata::UnsignedInteger32 nbEvtsCorrupted_;

    };

  } } // namespace evb::bu

#endif // _evb_bu_DiskWriter_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
