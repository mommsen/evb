#ifndef _evb_bu_StreamHandler_h_
#define _evb_bu_StreamHandler_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "evb/OneToOneQueue.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/LumiMonitor.h"


namespace evb {

  namespace bu { // namespace evb::bu

    /**
     * \ingroup xdaqApps
     * \brief Handle a stream of events to be written to disk
     */

    class StreamHandler
    {
    public:

      StreamHandler
      (
        const uint32_t buInstance,
        const uint32_t runNumber,
        const boost::filesystem::path& buRawDataDir,
        const boost::filesystem::path& buMetaDataDir,
        ConfigurationPtr
      );

      ~StreamHandler();

      /**
       * Write the given event to disk
       */
      void writeEvent(const EventPtr);

      /**
       * Close the stream
       */
      void close();

      /**
       * Get the next available lumi monitor.
       * Return false if no lumi monitor is available
       */
      bool getLumiMonitor(LumiMonitorPtr&);


    private:

      void closeLumiSection(const uint32_t lumiSection);

      const uint32_t buInstance_;
      const uint32_t runNumber_;

      const boost::filesystem::path runRawDataDir_;
      const boost::filesystem::path runMetaDataDir_;
      const ConfigurationPtr configuration_;

      FileHandlerPtr fileHandler_;

      LumiMonitorPtr currentLumiMonitor_;
      boost::mutex currentLumiMonitorMutex_;
      typedef OneToOneQueue<LumiMonitorPtr> LumiMonitorFIFO;
      LumiMonitorFIFO lumiMonitorFIFO_;

    };

    typedef boost::shared_ptr<StreamHandler> StreamHandlerPtr;

  } } // namespace evb::bu

#endif // _evb_bu_StreamHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
