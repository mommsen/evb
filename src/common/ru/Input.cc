#include "evb/RU.h"
#include "evb/ru/Input.h"
#include "evb/DumpUtility.h"
#include "evb/Exception.h"


evb::ru::Input::Input
(
  boost::shared_ptr<RU> ru,
  SuperFragmentTablePtr superFragmentTable
) :
ru_(ru),
superFragmentTable_(superFragmentTable),
handler_(new FEROLproxy(ru,superFragmentTable) )
{
  resetMonitoringCounters();
}


void evb::ru::Input::inputSourceChanged()
{
  const std::string inputSource = inputSource_.toString();

  LOG4CPLUS_INFO(ru_->getApplicationLogger(), "Setting input source to " + inputSource);

  if ( inputSource == "FEROL" )
  {
    handler_.reset( new FEROLproxy(ru_,superFragmentTable_) );
  }
  else if ( inputSource == "Local" )
  {
    handler_.reset( new DummyInputData(ru_,superFragmentTable_) );
  }
  else
  {
    XCEPT_RAISE(exception::Configuration,
      "Unknown input source " + inputSource + " requested.");
  }

  configure();
  resetMonitoringCounters();
}


void evb::ru::Input::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  dumpFragmentToLogger(bufRef);
  handler_->I2Ocallback(bufRef);
}


void evb::ru::Input::dumpFragmentToLogger(toolbox::mem::Reference* bufRef) const
{
  if ( ! dumpFragmentsToLogger_.value_ ) return;

  std::stringstream oss;
  DumpUtility::dump(oss, bufRef);
  LOG4CPLUS_INFO(ru_->getApplicationLogger(), oss.str());
}


void evb::ru::Input::configure()
{
  InputHandler::Configuration conf;
  conf.dummyFedPayloadSize = dummyFedPayloadSize_.value_;
  conf.dummyFedPayloadStdDev = dummyFedPayloadStdDev_.value_;
  conf.fedSourceIds = fedSourceIds_;
  conf.usePlayback = usePlayback_.value_;
  conf.playbackDataFile = playbackDataFile_.value_;
  handler_->configure(conf);
}


void evb::ru::Input::clear()
{
  handler_->clear();
}


void evb::ru::Input::appendConfigurationItems(InfoSpaceItems& params)
{
  inputSource_ = "FBO";
  dumpFragmentsToLogger_ = false;
  usePlayback_ = false;
  playbackDataFile_ = "";  
  dummyFedPayloadSize_ = 2048;
  dummyFedPayloadStdDev_ = 0;
  
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
  inputParams_.add("usePlayback", &usePlayback_);
  inputParams_.add("playbackDataFile", &playbackDataFile_);
  inputParams_.add("dummyFedPayloadSize", &dummyFedPayloadSize_);
  inputParams_.add("dummyFedPayloadStdDev", &dummyFedPayloadStdDev_);
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
  lastEventNumberFromRUI_ = handler_->lastEventNumber();
  i2oEVMRUDataReadyCount_ = handler_->fragmentsCount();
}


void evb::ru::Input::resetMonitoringCounters()
{
  handler_->resetMonitoringCounters();
}


uint64_t evb::ru::Input::fragmentsCount() const
{
  return handler_->fragmentsCount();
}


void evb::ru::Input::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUInput - " << inputSource_.toString() << "</p>"    << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  handler_->printHtml(out);

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
