#ifndef _evb_bu_FileHandler_h_
#define _evb_bu_FileHandler_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <string.h>

#include "evb/bu/Event.h"


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
       * Write the event to disk
       */
      void writeEvent(const EventPtr&);

      /**
       * Close the file and do the bookkeeping.
       */
      uint64_t closeAndGetFileSize();

    private:

      const std::string rawFileName_;
      int fileDescriptor_;

      uint64_t fileSize_;

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
