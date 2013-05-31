#ifndef _evb_bu_ResourceManager_h_
#define _evb_bu_ResourceManager_h_

#include <map>
#include <stdint.h>

#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "evb/InfoSpaceItems.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"


namespace evb {

  class BU;
  
  namespace bu { // namespace evb::bu
    
    class StateMachine;
    
    /**
     * \ingroup xdaqApps
     * \brief Manage resources available to the BU
     */
    
    class ResourceManager : public toolbox::lang::Class
    {
      
    public:
      
      ResourceManager
      (
        BU*
      );
      
      virtual ~ResourceManager() {};
      
      /**
       * Mark the resource contained in the passed data block as under construction
       */
      void underConstruction(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*);

      /**
       * Mark the resource contained in the passed data block as complete
       */
      void completed(const msg::I2O_DATA_BLOCK_MESSAGE_FRAME*);
      
      /**
       * Append the info space parameters used for the
       * configuration to the InfoSpaceItems
       */
      void appendConfigurationItems(InfoSpaceItems&);
      
      /**
       * Append the info space items to be published in the 
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);
      
      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();
      
      /**
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();
      
      /**
       * Configure
       */
      void configure();
      
      /**
       * Remove all data
       */
      void clear();
            
      /**
       * Register the state machine
       */
      void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
      { stateMachine_ = stateMachine; }
      
      /**
       * Start processing messages
       */
      void startProcessing();

      /**
       * Stop processing messages
       */
      void stopProcessing();
      
      /**
       * Print monitoring/configuration as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      
    private:
      
      void startProcessingWorkLoop();
      bool process(toolbox::task::WorkLoop*);
      
      BU* bu_;
      boost::shared_ptr<StateMachine> stateMachine_;
      
      bool doProcessing_;
      bool processActive_;
      
      toolbox::task::WorkLoop* processingWL_;
      toolbox::task::ActionSignature* processingAction_;
      
      InfoSpaceItems resourceManagerParams_;
      
    };
    
    
  } } //namespace evb::bu

#endif // _evb_bu_ResourceManager_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
