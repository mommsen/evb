#ifndef _evb_bu_StreamHandler_h_
#define _evb_bu_StreamHandler_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "evb/OneToOneQueue.h"
#include "evb/bu/Configuration.h"
#include "evb/bu/Event.h"
#include "evb/bu/FileHandler.h"
#include "evb/bu/FileStatistics.h"


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

      /**
       * Write the given event to disk
       */
      void writeEvent(const EventPtr&);

      /**
       * Close all open files
       */
      void closeFiles();

      /**
       * Close all files opened before the given time.
       * Return true if a file was closed
       */
      bool closeFilesIfOpenedBefore(const time_t&);

      /**
       * Get the next available statistics of a closed file.
       * Return false if no statistics is available
       */
      bool getFileStatistics(FileStatisticsPtr&);


    private:

      FileHandlerPtr getFileHandlerForLumiSection(const uint32_t lumiSection);
      FileHandlerPtr getNewFileHandler(const uint32_t lumiSection);
      void closeFileHandler(const FileHandlerPtr&);

      const std::string streamFileName_;
      const ConfigurationPtr configuration_;
      uint8_t index_;

      typedef std::map<uint32_t,FileHandlerPtr> FileHandlers;
      FileHandlers fileHandlers_;
      boost::mutex fileHandlersMutex_;

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
