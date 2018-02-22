#include <iomanip>
#include <sstream>

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
  dipFactory_(Dip::create())
{
  dipFactory_->setDNSNode( dipNodes.c_str() );

  // Attention: DCS HV values have to come first and in the order of the HV bits
  // defined in DataFormats::OnlineMetaData::DCSRecord::Partition in CMSSW
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_BP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_BM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_EP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_EM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBa/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBb/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HEHBc/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HF/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_HO/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_RPC/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DT0/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DTP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_DT/CMS_DT_DTM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_CSC/CMS_CSCP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_CSC/CMS_CSCM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_CASTOR/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_HCAL/CMS_HCAL_ZDC/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TIB_TID/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TOB/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TECP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_TRACKER/CMS_TRACKER_TECM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_PIXEL/CMS_PIXEL_BPIX/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_PIXEL/CMS_PIXEL_FPIX/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_ESP/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/DCS/CMS_ECAL/CMS_ECAL_ESM/state",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/MCS/Current",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/BRIL/Luminosity",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/Tracker/BeamSpot",unavailable) );
  dipTopics_.push_back( DipTopic("dip/CMS/CTPPS/detectorFSM",unavailable) );
}


evb::readoutunit::MetaDataRetriever::~MetaDataRetriever()
{
  for ( DipSubscriptions::iterator it = dipSubscriptions_.begin(), itEnd = dipSubscriptions_.end();
        it != itEnd; ++it)
  {
    dipFactory_->destroyDipSubscription(*it);
  }
}


void evb::readoutunit::MetaDataRetriever::subscribeToDip(const std::string& maskedDipTopics)
{
  for ( DipTopics::iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it )
  {
    if ( maskedDipTopics.find(it->first) != std::string::npos )
      it->second = masked;
    else if ( it->second != okay )
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
    pos->second = okay;
}


void evb::readoutunit::MetaDataRetriever::disconnected(DipSubscription* subscription, char* message)
{
  std::ostringstream msg;
  msg << "Disconnected from " << subscription->getTopicName() << ": " << message;
  LOG4CPLUS_ERROR(logger_, msg.str());

  const DipTopics::iterator pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic( subscription->getTopicName() ));
  if ( pos != dipTopics_.end() )
    pos->second = unavailable;
}


void evb::readoutunit::MetaDataRetriever::handleException(DipSubscription* subscription, DipException& exception)
{
  std::ostringstream msg;
  msg << "Exception for " << subscription->getTopicName() << ": " << exception.what();
  LOG4CPLUS_ERROR(logger_, msg.str());

  const DipTopics::iterator pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic( subscription->getTopicName() ));
  if ( pos != dipTopics_.end() )
    pos->second = unavailable;
}


void evb::readoutunit::MetaDataRetriever::handleMessage(DipSubscription* subscription, DipData& dipData)
{
  const std::string topicName = subscription->getTopicName();
  if ( topicName == "dip/CMS/BRIL/Luminosity" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(luminosityMutex_);

      lastLuminosity_.timeStamp = dipData.extractDipTime().getAsMillis();
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

      lastBeamSpot_.timeStamp = dipData.extractDipTime().getAsMillis();
      lastBeamSpot_.x = dipData.extractFloat("x");
      lastBeamSpot_.y = dipData.extractFloat("y");
      lastBeamSpot_.z = dipData.extractFloat("z");
      lastBeamSpot_.widthX = dipData.extractFloat("width_x");
      lastBeamSpot_.widthY = dipData.extractFloat("width_y");
      lastBeamSpot_.sigmaZ = dipData.extractFloat("sigma_z");

      lastBeamSpot_.errX = dipData.extractFloat("err_x");
      lastBeamSpot_.errY = dipData.extractFloat("err_y");
      lastBeamSpot_.errX = dipData.extractFloat("err_z");
      lastBeamSpot_.errWidthX = dipData.extractFloat("err_width_x");
      lastBeamSpot_.errWidthY = dipData.extractFloat("err_width_y");
      lastBeamSpot_.errSigmaZ = dipData.extractFloat("err_sigma_z");

      lastBeamSpot_.dxdz = dipData.extractFloat("dxdz");
      lastBeamSpot_.dydz = dipData.extractFloat("dydz");
      lastBeamSpot_.errDxdz = dipData.extractFloat("err_dxdz");
      lastBeamSpot_.errDydz = dipData.extractFloat("err_dydz");
    }
  }
  else if ( topicName == "dip/CMS/CTPPS/detectorFSM" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(ctppsMutex_);

      lastCTPPS_.timeStamp = dipData.extractDipTime().getAsMillis();
      lastCTPPS_.status = 0;

      for (uint8_t i = 0; i < MetaData::CTPPS::rpCount; ++i)
      {
        uint64_t status = dipData.extractInt(MetaData::CTPPS::ctppsRP[i]);
        lastCTPPS_.status |= (status & 0x3) << (i*2);
      }
    }
  }
  else if ( topicName == "dip/CMS/MCS/Current" )
  {
    if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(dcsMutex_);

      const uint64_t dipTime = dipData.extractDipTime().getAsMillis();
      lastDCS_.timeStamp = std::max(lastDCS_.timeStamp,dipTime);
      lastDCS_.magnetCurrent = dipData.extractFloat();
    }
  }
  else
  {
    const uint16_t pos = std::find_if(dipTopics_.begin(), dipTopics_.end(), isTopic(topicName)) - dipTopics_.begin();
    if ( pos < dipTopics_.size() && dipData.extractDataQuality() == DIP_QUALITY_GOOD )
    {
      boost::mutex::scoped_lock sl(dcsMutex_);

      const std::string state = dipData.extractString();

      if ( state.find("READY") != std::string::npos || state.find("ON") != std::string::npos )
        lastDCS_.highVoltageReady |= (1 << pos);
      else
        lastDCS_.highVoltageReady &= ~(1 << pos);

      const uint64_t dipTime = dipData.extractDipTime().getAsMillis();
      lastDCS_.timeStamp = std::max(lastDCS_.timeStamp,dipTime);
    }
  }
}


bool evb::readoutunit::MetaDataRetriever::fillLuminosity(MetaData::Luminosity& luminosity)
{
  boost::mutex::scoped_lock sl(luminosityMutex_);

  if ( lastLuminosity_.timeStamp > 0 && lastLuminosity_ != luminosity )
  {
    luminosity = lastLuminosity_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillBeamSpot(MetaData::BeamSpot& beamSpot)
{
  boost::mutex::scoped_lock sl(beamSpotMutex_);

  if ( lastBeamSpot_.timeStamp > 0 && lastBeamSpot_ != beamSpot )
  {
    beamSpot = lastBeamSpot_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillCTPPS(MetaData::CTPPS& CTPPS)
{
  boost::mutex::scoped_lock sl(ctppsMutex_);

  if ( lastCTPPS_.timeStamp > 0 && lastCTPPS_ != CTPPS )
  {
    CTPPS = lastCTPPS_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillDCS(MetaData::DCS& dcs)
{
  boost::mutex::scoped_lock sl(dcsMutex_);

  if ( lastDCS_.timeStamp > 0 && lastDCS_ != dcs )
  {
    dcs = lastDCS_;
    return true;
  }

  return false;
}


bool evb::readoutunit::MetaDataRetriever::fillData(unsigned char* payload)
{
  MetaData::Data* data = (MetaData::Data*)payload;
  data->version = MetaData::version;

  return (
    fillLuminosity(data->luminosity) ||
    fillBeamSpot(data->beamSpot) ||
    fillCTPPS(data->ctpps) ||
    fillDCS(data->dcs)
  );
}


bool evb::readoutunit::MetaDataRetriever::missingSubscriptions() const
{
  for ( DipTopics::const_iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it)
  {
    if ( it->second == unavailable ) return true;
  }
  return false;
}


void evb::readoutunit::MetaDataRetriever::addListOfSubscriptions(std::ostringstream& msg, const bool missingOnly)
{
  for ( DipTopics::const_iterator it = dipTopics_.begin(), itEnd = dipTopics_.end();
        it != itEnd; ++it)
  {
    if ( missingOnly )
    {
      if ( it->second == unavailable )
        msg << " " << it->first;
    }
    else
    {
      msg << " " << it->first;
    }
  }
}


cgicc::td evb::readoutunit::MetaDataRetriever::getDipStatus(const std::string& urn) const
{
  using namespace cgicc;

  td status;
  status.set("colspan","6").set("style","text-align:center");
  status.add(a(std::string("soft FED ") + (missingSubscriptions()?"missing DIP subscriptions":"subscribed to DIP"))
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
            .add(th("value")));

  for (uint16_t pos = 0; pos < dipTopics_.size(); ++pos)
  {
    const std::string topic = dipTopics_[pos].first;

    switch ( dipTopics_[pos].second )
    {
      case okay :
      {
        std::ostringstream valueStr;

        if ( topic == "dip/CMS/BRIL/Luminosity" )
        {
          boost::mutex::scoped_lock sl(luminosityMutex_);
          valueStr << lastLuminosity_;
        }
        else if ( topic == "dip/CMS/Tracker/BeamSpot" )
        {
          boost::mutex::scoped_lock sl(dcsMutex_);
          valueStr << lastBeamSpot_;
        }
        else if ( topic == "dip/CMS/CTPPS/detectorFSM" )
        {
          boost::mutex::scoped_lock sl(ctppsMutex_);
          valueStr << lastCTPPS_;
        }
        else if ( topic == "dip/CMS/MCS/Current" )
        {
          boost::mutex::scoped_lock sl(dcsMutex_);
          valueStr << std::fixed << std::setprecision(3) << lastDCS_.magnetCurrent << " A";
        }
        else
        {
          boost::mutex::scoped_lock sl(dcsMutex_);
          if ( lastDCS_.highVoltageReady & (1 << pos) )
            valueStr << "READY";
          else
            valueStr << "OFF";
        }

        table.add(tr()
                  .add(td(topic))
                  .add(td(valueStr.str())));
        break;
      }
      case unavailable:
      {
        table.add(tr().set("style","background-color: #ff9380")
                  .add(td(topic))
                  .add(td("unavailable")));
        break;
      }
      case masked:
      {
        table.add(tr().set("style","color: #c1c1c1")
                  .add(td(topic))
                  .add(td("masked")));
        break;
      }
    }
  }

  return table;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
