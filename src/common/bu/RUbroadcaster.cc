#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "evb/Exception.h"
#include "evb/Constants.h"
#include "evb/bu/RUbroadcaster.h"
#include "evb/UnsignedInteger32Less.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


evb::bu::RUbroadcaster::RUbroadcaster
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
tid_(0)
{}


void evb::bu::RUbroadcaster::initRuInstances(InfoSpaceItems& params)
{
  getRuInstances();

  params.add("ruInstances", &ruInstances_);
}


void evb::bu::RUbroadcaster::getRuInstances()
{
  boost::mutex::scoped_lock sl(ruInstancesMutex_);

  std::set<xdaq::ApplicationDescriptor*> ruDescriptors;
  
  try
  {
    ruDescriptors =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("evb::RU");
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_WARN(logger_,
      "There are no RU application descriptors : "
      << xcept::stdformat_exception_history(e));
    
    XCEPT_DECLARE_NESTED(exception::Configuration,
      sentinelException,
      "There are no RU application descriptors", e);
    app_->notifyQualified("warn",sentinelException);
    
    ruDescriptors.clear();
  }

  ruInstances_.clear();
  
  for (std::set<xdaq::ApplicationDescriptor*>::const_iterator
         it=ruDescriptors.begin(), itEnd =ruDescriptors.end();
       it != itEnd; ++it)
  {
    ruInstances_.push_back((*it)->getInstance());
  }

  std::sort(ruInstances_.begin(), ruInstances_.end(),
    UnsignedInteger32Less());
}


void evb::bu::RUbroadcaster::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(app_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }
  
  try
  {
    fillParticipatingRUsUsingRuInstances();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to fill RU descriptor list using \"ruInstances\"", e);
  }
}


void evb::bu::RUbroadcaster::fillParticipatingRUsUsingRuInstances()
{  
  boost::mutex::scoped_lock sl(ruInstancesMutex_);

  // Clear list of participating RUs
  participatingRUs_.clear();
  ruTids_.clear();

  // Fill list of participating RUs
  for (RUInstances::iterator it = ruInstances_.begin(),
         itEnd = ruInstances_.end();
       it != itEnd; ++it)
  {
    fillRUInstance(*it);
  }
}


void evb::bu::RUbroadcaster::fillRUInstance(xdata::UnsignedInteger32 instance)
{
  ApplicationDescriptorAndTid ru;
  
  try
  {
    ru.descriptor =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptor("evb::RU", instance);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor for RU ";
    oss << instance.toString();
    
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
  
  try
  {
    ru.tid = i2o::utils::getAddressMap()->getTid(ru.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get I2O TID for RU ";
    oss << instance.toString();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }
  
  if ( ! participatingRUs_.insert(ru).second )
  {
    std::stringstream oss;
    
    oss << "Participating RU instance is a duplicate.";
    oss << " Instance:" << instance.toString();
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  
  ruTids_.push_back(ru.tid);
}

void evb::bu::RUbroadcaster::sendToAllRUs
(
  toolbox::mem::Reference* bufRef,
  const size_t bufSize
)
{
  ////////////////////////////////////////////////////////////
  // Make a copy of the pairs message under construction    //
  // for each RU and send each copy to its corresponding RU //
  ////////////////////////////////////////////////////////////

  for ( RUDescriptorsAndTids::const_iterator it = participatingRUs_.begin(),
          itEnd = participatingRUs_.end();
        it != itEnd; ++it)
  {
    // Create an empty request message
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, bufSize);
    char* copyFrame  = (char*)(copyBufRef->getDataLocation());

    // Copy the message under construction into
    // the newly created empty message
    memcpy(
      copyFrame,
      bufRef->getDataLocation(),
      bufRef->getDataSize()
    );

    // Set the size of the copy
    copyBufRef->setDataSize(bufSize);

    // Set the I2O TID target address
    ((I2O_MESSAGE_FRAME*)copyFrame)->TargetAddress = it->tid;

    // Send the pairs message to the RU
    try
    {
      app_->getApplicationContext()->
        postFrame(
          copyBufRef,
          app_->getApplicationDescriptor(),
          it->descriptor //,
          //i2oExceptionHandler_,
          //it->descriptor
        );
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to send message to RU";
      oss << it->descriptor->getInstance();

      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
