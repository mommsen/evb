#include "evb/BU.h"
#include "evb/bu/ResourceManager.h"
#include "evb/bu/StateMachine.h"
#include "evb/Constants.h"
#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <string.h>


evb::bu::ResourceManager::ResourceManager
(
  BU* bu
) :
bu_(bu),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void evb::bu::ResourceManager::startProcessing()
{
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void evb::bu::ResourceManager::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


void evb::bu::ResourceManager::startProcessingWorkLoop()
{
  try
  {
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( bu_->getIdentifier("ResourceManagerProcessing"), "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &evb::bu::ResourceManager::process,
          bu_->getIdentifier("resourceManagerProcess") );
    
      processingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'ResourceManagerProcessing'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool evb::bu::ResourceManager::process(toolbox::task::WorkLoop*)
{
  ::usleep(1000);
  
  processActive_ = true;
  
  try
  {
    while ( doProcessing_ )
    {
      // uint32_t buResourceId;
      // std::vector<EvBid> evbIds;
    };
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( Fail(e) );
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


void evb::bu::ResourceManager::appendConfigurationItems(InfoSpaceItems& params)
{
}


void evb::bu::ResourceManager::appendMonitoringItems(InfoSpaceItems& items)
{
}


void evb::bu::ResourceManager::updateMonitoringItems()
{
}


void evb::bu::ResourceManager::resetMonitoringCounters()
{
}


void evb::bu::ResourceManager::configure()
{
}


void evb::bu::ResourceManager::clear()
{
}


void evb::bu::ResourceManager::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>ResourceManager</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  resourceManagerParams_.printHtml("Configuration", out);
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
