#ifndef _evb_readoutunit_MetaDataRetriever_h_
#define _evb_readoutunit_MetaDataRetriever_h_

#include <stdint.h>
#include <string.h>
#include <utility>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "dip/DipException.h"
#include "dip/DipFactory.h"
#include "dip/DipSubscription.h"
#include "dip/DipSubscriptionListener.h"

#include "cgicc/HTMLClasses.h"
#include "evb/readoutunit/Scalers.h"
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

      cgicc::td getDipStatus(const std::string& urn) const;
      cgicc::table dipStatusTable() const;


    private:

      void subscribeToDip();
      bool fillLuminosity(Scalers::Luminosity&);
      bool fillBeamSpot(Scalers::BeamSpot&);
      bool fillDCS(Scalers::DCS&);

      log4cplus::Logger& logger_;

      boost::scoped_ptr<DipFactory> dipFactory_;
      typedef std::vector<DipSubscription*> DipSubscriptions;
      DipSubscriptions dipSubscriptions_;

      typedef std::pair<std::string,bool> DipTopic;
      struct isTopic
      {
        isTopic(const std::string& topic) : topic_(topic) {};
        bool operator()(const DipTopic& p)
        {
          return (p.first == topic_);
        }
        const std::string topic_;
      };
      typedef std::vector<DipTopic> DipTopics;
      DipTopics dipTopics_;

      Scalers::Luminosity lastLuminosity_;
      mutable boost::mutex luminosityMutex_;

      Scalers::BeamSpot lastBeamSpot_;
      mutable boost::mutex beamSpotMutex_;

      Scalers::DCS lastDCS_;
      mutable boost::mutex dcsMutex_;

    };

    typedef boost::shared_ptr<MetaDataRetriever> MetaDataRetrieverPtr;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_MetaDataRetriever_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
