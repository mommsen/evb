#include <sstream>

#include "evb/RU.h"
#include "evb/ru/Input.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"


evb::ru::Input::Input(RU* ru) :
ru_(ru),
handler_(new FEROLproxy() ),
acceptI2Omessages_(false)
{
  resetMonitoringCounters();
}


void evb::ru::Input::inputSourceChanged()
{
  const std::string inputSource = inputSource_.toString();

  LOG4CPLUS_INFO(ru_->getApplicationLogger(), "Setting input source to " + inputSource);

  if ( inputSource == "FEROL" )
  {
    handler_.reset( new FEROLproxy() );
  }
  else if ( inputSource == "Local" )
  {
    handler_.reset( new DummyInputData() );
  }
  else
  {
    XCEPT_RAISE(exception::Configuration,
      "Unknown input source " + inputSource + " requested.");
  }

  configure();
  resetMonitoringCounters();
}


void evb::ru::Input::dataReadyCallback(toolbox::mem::Reference* bufRef)
{
  if ( acceptI2Omessages_ )
  {
    updateInputCounters(bufRef);
    dumpFragmentToLogger(bufRef);
    handler_->dataReadyCallback(bufRef);
  }
}


void evb::ru::Input::updateInputCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);
  
  const I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  const unsigned char* payload = (unsigned char*)frame + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const ferolh_t* ferolHeader = (ferolh_t*)payload;
  assert( ferolHeader->signature() == FEROL_SIGNATURE );
  const uint64_t fedId = ferolHeader->fed_id();
  const uint64_t eventNumber = ferolHeader->event_number();
  
  InputMonitors::iterator pos = inputMonitors_.find(fedId);
  if ( pos == inputMonitors_.end() )
  {
    std::ostringstream msg;
    msg << "The received FED id " << fedId;
    msg << " for event " << eventNumber;
    msg << " is not in the excepted FED list: ";
    std::copy(fedSourceIds_.begin(), fedSourceIds_.end(),
      std::ostream_iterator<uint16_t>(msg," "));
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  
  pos->second.lastEventNumber = eventNumber;
  pos->second.perf.sumOfSizes += frame->totalLength;
  pos->second.perf.sumOfSquares += frame->totalLength*frame->totalLength;
  ++(pos->second.perf.i2oCount);
  if ( ferolHeader->is_last_packet() )
    ++(pos->second.perf.logicalCount);
}


void evb::ru::Input::dumpFragmentToLogger(toolbox::mem::Reference* bufRef) const
{
  if ( ! dumpFragmentsToLogger_.value_ ) return;

  std::stringstream oss;
  DumpUtility::dump(oss, bufRef);
  LOG4CPLUS_INFO(ru_->getApplicationLogger(), oss.str());
}


bool evb::ru::Input::getNextAvailableSuperFragment(FragmentChainPtr superFragment)
{
  return handler_->getNextAvailableSuperFragment(superFragment);
}


bool evb::ru::Input::getSuperFragmentWithEvBid(const EvBid& evbId, FragmentChainPtr superFragment)
{
  return handler_->getSuperFragmentWithEvBid(evbId,superFragment);
}


void evb::ru::Input::configure()
{
  InputHandler::Configuration conf;
  conf.dropInputData = dropInputData_.value_;
  conf.dummyFedSize = dummyFedSize_.value_;
  conf.dummyFedSizeStdDev = dummyFedSizeStdDev_.value_;
  conf.fragmentPoolSize = fragmentPoolSize_.value_;
  conf.fedSourceIds = fedSourceIds_;
  conf.usePlayback = usePlayback_.value_;
  conf.playbackDataFile = playbackDataFile_.value_;
  handler_->configure(conf);
}


void evb::ru::Input::clear()
{
  acceptI2Omessages_ = false;
  handler_->clear();
}


void evb::ru::Input::appendConfigurationItems(InfoSpaceItems& params)
{
  inputSource_ = "FEROL";
  dumpFragmentsToLogger_ = false;
  dropInputData_ = false;
  usePlayback_ = false;
  playbackDataFile_ = "";  
  dummyFedSize_ = 2048;
  dummyFedSizeStdDev_ = 0;
  fragmentPoolSize_ = 1638400;
  
  // Default is 8 FEDs per super-fragment
  // Trigger has FED source id 0, RU0 has 1 to 8, RU1 has 9 to 16, etc.
  const uint32_t instance = ru_->getApplicationDescriptor()->getInstance();
  const uint32_t firstSourceId = (instance * 8) + 1;
  const uint32_t lastSourceId  = (instance * 8) + 8;

  for (uint32_t sourceId=firstSourceId; sourceId<=lastSourceId; ++sourceId)
  {
    fedSourceIds_.push_back(sourceId);
  }
  
  inputParams_.clear();
  inputParams_.add("inputSource", &inputSource_, InfoSpaceItems::change);
  inputParams_.add("dumpFragmentsToLogger", &dumpFragmentsToLogger_);
  inputParams_.add("dropInputData", &dropInputData_);
  inputParams_.add("usePlayback", &usePlayback_);
  inputParams_.add("playbackDataFile", &playbackDataFile_);
  inputParams_.add("dummyFedSize", &dummyFedSize_);
  inputParams_.add("dummyFedSizeStdDev", &dummyFedSizeStdDev_);
  inputParams_.add("fragmentPoolSize", &fragmentPoolSize_);
  inputParams_.add("fedSourceIds", &fedSourceIds_);

  params.add(inputParams_);
}


void evb::ru::Input::appendMonitoringItems(InfoSpaceItems& items)
{
  lastEventNumberFromRUI_ = 0;
  i2oEVMRUDataReadyCount_ = 0;

  items.add("lastEventNumberFromRUI", &lastEventNumberFromRUI_);
  items.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_);
}


void evb::ru::Input::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);
  
  PerformanceMonitor performanceMonitor;
  uint32_t lastEventNumber = 0;
  uint32_t dataReadyCount = 0;
  for (InputMonitors::iterator it = inputMonitors_.begin(), itEnd = inputMonitors_.end();
       it != itEnd; ++it)
  {
    if ( lastEventNumber < it->second.lastEventNumber )
      lastEventNumber = it->second.lastEventNumber;
    dataReadyCount += it->second.perf.logicalCount;
    
    it->second.rate = it->second.perf.logicalRate();
    it->second.bandwidth = it->second.perf.bandwidth();
    it->second.perf.reset();
  }
  lastEventNumberFromRUI_ = lastEventNumber;
  i2oEVMRUDataReadyCount_ = dataReadyCount;
}


void evb::ru::Input::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);
    
  inputMonitors_.clear();
  for (xdata::Vector<xdata::UnsignedInteger32>::const_iterator it = fedSourceIds_.begin(), itEnd = fedSourceIds_.end();
        it != itEnd; ++it)
  {
    InputMonitoring monitor;
    monitor.lastEventNumber = 0;
    monitor.rate = 0;
    monitor.bandwidth = 0;
    inputMonitors_.insert(InputMonitors::value_type(it->value_,monitor));
  }
}


void evb::ru::Input::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUInput - " << inputSource_.toString() << "</p>"    << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">Statistics per FED</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>FED id</td>"                                       << std::endl;
  *out << "<td>Last event</td>"                                   << std::endl;
  *out << "<td>Rate (kHz)</td>"                                   << std::endl;
  *out << "<td>B/w (MB/s)</td>"                                   << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  const std::_Ios_Fmtflags originalFlags=out->flags();
  const int originalPrecision=out->precision();
  out->setf(std::ios::scientific);
  out->setf(std::ios::fixed);
  out->precision(2);
  
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);
  
  InputMonitors::const_iterator it, itEnd;
  for (it=inputMonitors_.begin(), itEnd = inputMonitors_.end();
       it != itEnd; ++it)
  {
    *out << "<tr>"                                                << std::endl;
    *out << "<td>" << it->first << "</td>"                        << std::endl;
    *out << "<td>" << it->second.lastEventNumber << "</td>"       << std::endl;
    *out << "<td>" << it->second.rate / 1000 << "</td>"           << std::endl;
    *out << "<td>" << it->second.bandwidth / 0x100000 << "</td>"  << std::endl;
    *out << "</tr>"                                               << std::endl;
  }
  
  out->flags(originalFlags);
  out->precision(originalPrecision);
  
  *out << "</table>"                                              << std::endl;
  
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  inputParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
