#ifndef _evb_readoutunit_MetaDataRetriever_h_
#define _evb_readoutunit_MetaDataRetriever_h_

#include <memory>
#include <mutex>
#include <sstream>
#include <stdint.h>
#include <string.h>
#include <utility>
#include <vector>

#include "dip/DipException.h"
#include "dip/DipFactory.h"
#include "dip/DipSubscription.h"
#include "dip/DipSubscriptionListener.h"

#include "cgicc/HTMLClasses.h"
#include "evb/readoutunit/MetaData.h"
#include "log4cplus/logger.h"


namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Retreive meta-data to be injected into the event stream
     */

    class MetaDataRetriever : public DipSubscriptionListener
    {
    public:

      MetaDataRetriever(log4cplus::Logger&, const std::string& identifier, const std::string& dipNodes);
      ~MetaDataRetriever();

      bool fillData(unsigned char*);

      // DIP listener callbacks
      void handleMessage(DipSubscription*, DipData&);
      void disconnected(DipSubscription*, char*);
      void connected(DipSubscription*);
      void handleException(DipSubscription*, DipException&);

      void subscribeToDip(const std::string& maskedDipTopics);
      bool missingSubscriptions() const;
      void addListOfSubscriptions(std::ostringstream&, const bool missingOnly = false);

      cgicc::td getDipStatus(const std::string& urn) const;
      cgicc::table dipStatusTable() const;


    private:

      bool fillLuminosity(MetaData::Luminosity&);
      bool fillBeamSpot(MetaData::BeamSpot&);
      bool fillCTPPS(MetaData::CTPPS&);
      bool fillDCS(MetaData::DCS&);

      log4cplus::Logger& logger_;

      std::unique_ptr<DipFactory> dipFactory_;
      using DipSubscriptions = std::vector<DipSubscription*>;
      DipSubscriptions dipSubscriptions_;

      enum DipStatus { unavailable,okay,masked };
      using DipTopic = std::pair<std::string,DipStatus>;
      struct isTopic
      {
        isTopic(const std::string& topic) : topic_(topic) {};
        bool operator()(const DipTopic& p)
        {
          return (p.first == topic_);
        }
        const std::string topic_;
      };
      using DipTopics = std::vector<DipTopic>;
      DipTopics dipTopics_;

      MetaData::Luminosity lastLuminosity_;
      mutable std::mutex luminosityMutex_;

      MetaData::BeamSpot lastBeamSpot_;
      mutable std::mutex beamSpotMutex_;

      MetaData::CTPPS lastCTPPS_;
      mutable std::mutex ctppsMutex_;

      MetaData::DCS lastDCS_;
      mutable std::mutex dcsMutex_;
    };

    using MetaDataRetrieverPtr = std::shared_ptr<MetaDataRetriever>;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_MetaDataRetriever_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
