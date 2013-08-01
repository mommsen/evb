#ifndef _evb_bu_StreamHandler_h_
#define _evb_bu_StreamHandler_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/scoped_ptr.hpp>

#include <stdint.h>

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
        const uint32_t buInstance,
        const uint32_t builderId,
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


    private:

      void closeLumiSection();

      const uint32_t buInstance_;
      const uint32_t builderId_;
      const uint32_t runNumber_;

      const boost::filesystem::path runRawDataDir_;
      const boost::filesystem::path runMetaDataDir_;
      const uint32_t maxEventsPerFile_;
      const uint32_t numberOfBuilders_;

      uint32_t currentLumiSection_;
      uint32_t index_;

      FileHandlerPtr fileHandler_;

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
