#ifndef _evb_bu_StreamHandler_h_
#define _evb_bu_StreamHandler_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "evb/OneToOneQueue.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"


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
        const std::string& streamFileName,
        ConfigurationPtr
      );

      ~StreamHandler();

      /**
       * Write the given event to disk
       */
      void writeEvent(const EventPtr);

      /**
       * Close the file
       */
      void closeFile();

      /**
       * Close the file if it was opened before the given time.
       * Return true if a file was closed
       */
      bool closeFileIfOpenedBefore(const time_t&);

      /**
       * Get the next available statistics of a closed file.
       * Return false if no statistics is available
       */
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
      typedef boost::shared_ptr<FileStatistics> FileStatisticsPtr;
      bool getFileStatistics(FileStatisticsPtr&);


    private:

      void do_closeFile();

      const std::string streamFileName_;
      const ConfigurationPtr configuration_;
      uint8_t index_;
      uint32_t currentLumiSection_;

      FileHandlerPtr fileHandler_;
      boost::mutex fileHandlerMutex_;

      FileStatisticsPtr currentFileStatistics_;
      typedef OneToOneQueue<FileStatisticsPtr> FileStatisticsFIFO;
      FileStatisticsFIFO fileStatisticsFIFO_;

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
