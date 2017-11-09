#include "dip/Dip.h"
#include "dip/DipData.h"
#include "dip/DipQuality.h"

#include "evb/readoutunit/MetaDataRetriever.h"
#include "log4cplus/loggingmacros.h"


evb::readoutunit::MetaDataRetriever::MetaDataRetriever(
  log4cplus::Logger& logger,
  const std::string& identifier,
  const std::string& dipNodes
) :
  logger_(logger),
  dipFactory_(Dip::create( identifier.c_str() ))
{
  dipFactory_->setDNSNode( dipNodes.c_str() );

  // Attention: DCS HV values have to come first and in the order of the HV bits
  // defined in Scalers::Data::highVoltageReady
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_BP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_BM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_EP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_EM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBa/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBb/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBc/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HF/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HO/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_RPC/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DT0/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DTP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DTM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_CSC/CMS_CSCP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_CSC/CMS_CSCM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_CASTOR/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_ZDC/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TIB_TID/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TOB/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TECP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TECM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_PIXEL/CMS_PIXEL_BPIX/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_PIXEL/CMS_PIXEL_FPIX/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_ESP/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_ESM/state",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/BRIL/Luminosity",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/Tracker/BeamSpot",false) );
  dipTopics_.push_back( DipTopic("dip/CMS/MCS/Current",false) );

  subscribeToDip();
}


evb::readoutunit::MetaDataRetriever::~MetaDataRetriever()
{
  for ( DipSubscriptions::iterator it = dipSubscriptions_.begin(), itEnd = dipSubscriptions_.end();
        it != itEnd; ++it)
  {
    dipFactory_->destroyDipSubscription(*it);
  }
}


void evb::readoutunit::MetaDataRetriever::subscribeToDip()
{
  for ( DipTopics::const_iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it )
  {
    if ( ! it->second )
      dipSubscriptions_.push_back( dipFactory_->createDipSubscription(it->first.c_str(),this) );
  }
}


void evb::readoutunit::MetaDataRetriever::connected(DipSubscription* subscription)
{
  std::ostringstream msg;
  msg << "Connected to " << subscription->getTopicName();
  LOG4CPLUS_INFO(logger_, msg.str());

  const DipTopics::iterator pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic( subscription->getTopicName() ));
  if ( pos != dipTopics_.end() )
    pos->second = true;
}


void evb::readoutunit::MetaDataRetriever::disconnected(DipSubscription* subscription, char* message)
{
  std::ostringstream msg;
  msg << "Disconnected from " << subscription->getTopicName() << ": " << message;
  LOG4CPLUS_WARN(logger_, msg.str());

  const DipTopics::iterator pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic( subscription->getTopicName() ));
  if ( pos != dipTopics_.end() )
    pos->second = false;
}


void evb::readoutunit::MetaDataRetriever::handleException(DipSubscription* subscription, DipException& exception)
{
  std::ostringstream msg;
  msg << "Exception for " << subscription->getTopicName() << ": " << exception.what();
  LOG4CPLUS_ERROR(logger_, msg.str());

  const DipTopics::iterator pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic( subscription->getTopicName() ));
  if ( pos != dipTopics_.end() )
    pos->second = false;
}


void evb::readoutunit::MetaDataRetriever::handleMessage(DipSubscription* subscription, DipData& dipData)
{
  const std::string topicName = subscription->getTopicName();
  if ( topicName == "dip/CMS/BRIL/Luminosity" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(luminosityMutex_);

      lastLuminosity_.valid = true;
      lastLuminosity_.lumiSection = dipData.extractInt("LumiSection");
      lastLuminosity_.lumiNibble = dipData.extractInt("LumiNibble");;
      lastLuminosity_.instLumi = dipData.extractFloat("InstLumi");
      lastLuminosity_.avgPileUp = dipData.extractFloat("AvgPileUp");
    }
  }
  else if ( topicName == "dip/CMS/Tracker/BeamSpot" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(beamSpotMutex_);

      lastBeamSpot_.valid = true;

      lastBeamSpot_.x = dipData.extractFloat("x");
      lastBeamSpot_.y = dipData.extractFloat("y");
      lastBeamSpot_.z = dipData.extractFloat("z");
      lastBeamSpot_.width_x = dipData.extractFloat("width_x");
      lastBeamSpot_.width_y = dipData.extractFloat("width_y");
      lastBeamSpot_.sigma_z = dipData.extractFloat("sigma_z");

      lastBeamSpot_.err_x = dipData.extractFloat("err_x");
      lastBeamSpot_.err_y = dipData.extractFloat("err_y");
      lastBeamSpot_.err_z = dipData.extractFloat("err_z");
      lastBeamSpot_.err_width_x = dipData.extractFloat("err_width_x");
      lastBeamSpot_.err_width_y = dipData.extractFloat("err_width_y");
      lastBeamSpot_.err_sigma_z = dipData.extractFloat("err_sigma_z");

      lastBeamSpot_.dxdz = dipData.extractFloat("dxdz");
      lastBeamSpot_.dydz = dipData.extractFloat("dydz");
      lastBeamSpot_.err_dxdz = dipData.extractFloat("err_dxdz");
      lastBeamSpot_.err_dydz = dipData.extractFloat("err_dydz");
    }
  }
  else if ( topicName == "dip/CMS/MCS/Current" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(magnetCurrentMutex_);

      lastMagnetCurrent_ = dipData.extractFloat(0);
      std::cout << "*** " << lastMagnetCurrent_ << std::endl;
    }
  }
  else
  {
    uint16_t pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic(topicName)) - dipTopics_.begin();
    if ( pos < dipTopics_.size() && dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      const std::string state = dipData.extractString();
      const bool ready = ( state.find("READY") != std::string::npos ||
                           state.find("ON") != std::string::npos );

      boost::mutex::scoped_lock sl(highVoltageReadyMutex_);
      lastHighVoltageReady_.set(pos,ready);
    }
  }
}


bool evb::readoutunit::MetaDataRetriever::fillLuminosity(Scalers::Luminosity& luminosity)
{
  boost::mutex::scoped_lock sl(luminosityMutex_);

  if ( lastLuminosity_.valid && lastLuminosity_ != luminosity )
  {
    luminosity = lastLuminosity_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillBeamSpot(Scalers::BeamSpot& beamSpot)
{
  boost::mutex::scoped_lock sl(beamSpotMutex_);

  if ( lastBeamSpot_.valid && lastBeamSpot_ != beamSpot )
  {
    beamSpot = lastBeamSpot_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillHighVoltageStatus(uint16_t& highVoltageReady)
{
  boost::mutex::scoped_lock sl(highVoltageReadyMutex_);

  const uint16_t oldReady = lastHighVoltageReady_.to_ulong();

  if ( highVoltageReady != oldReady )
  {
    highVoltageReady = oldReady;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillMagnetCurrent(float& magnetCurrent)
{
  boost::mutex::scoped_lock sl(magnetCurrentMutex_);

  if ( lastMagnetCurrent_ != magnetCurrent )
  {
    magnetCurrent = lastMagnetCurrent_;
    return true;
  }

  return false;
}


cgicc::td evb::readoutunit::MetaDataRetriever::getDipStatus(const std::string& urn) const
{
  using namespace cgicc;

  bool missingSubscriptions = false;

  for ( DipTopics::const_iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it)
  {
    if ( ! it->second )
    {
      missingSubscriptions = true;
    }
  }

  td status;
  status.set("colspan","6").set("style","text-align:center");
  status.add(a(std::string("soft FED ") + (missingSubscriptions?"missing DIP subscriptions":"subscribed to DIP"))
             .set("href","/"+urn+"/dipStatus").set("target","_blank"));

  return status;
}


cgicc::table evb::readoutunit::MetaDataRetriever::dipStatusTable() const
{
  using namespace cgicc;

  table table;
  table.set("class","xdaq-table-vertical");

  table.add(tr()
            .add(th("DIP topic"))
            .add(th("subscribed")));

  for ( DipTopics::const_iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it)
  {
    table.add(tr()
              .add(td(it->first))
              .add(td(it->second?"true":"false")));
  }

  return table;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
