#ifndef _evb_bu_StateMachine_h_
#define _evb_bu_StateMachine_h_

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/bu/EventTable.h"
#include "evb/Exception.h"
#include "evb/EvBStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"


namespace evb {
  
  class BU;
  
  namespace bu {
    
    class FUproxy;
    class RUproxy;
    class DiskWriter;
    class StateMachine;
    class Outermost;
    
    ///////////////////////
    // Transition events //
    ///////////////////////
    
    class BuCache : public boost::statechart::event<BuCache> 
    {
    public:
      BuCache(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getCacheMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
    };
    
    class BuAllocate : public boost::statechart::event<BuAllocate> 
    {
    public:
      BuAllocate(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getAllocateMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
    };
    
    class BuDiscard : public boost::statechart::event<BuDiscard> 
    {
    public:
      BuDiscard(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getDiscardMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
    };
    
    class EvmLumisection : public boost::statechart::event<EvmLumisection> 
    {
    public:
      EvmLumisection(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getLumisectionMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
    };
    
    ///////////////////////
    // The state machine //
    ///////////////////////
    
    typedef EvBStateMachine<StateMachine,bu::Outermost> EvBStateMachine;
    class StateMachine: public EvBStateMachine
    {
      
    public:
      
      StateMachine
      (
        boost::shared_ptr<BU>,
        boost::shared_ptr<FUproxy>,
        boost::shared_ptr<RUproxy>,
        boost::shared_ptr<DiskWriter>,
        EventTablePtr
      );
      
      void buAllocate(toolbox::mem::Reference*);
      void buCache(toolbox::mem::Reference*);
      void buDiscard(toolbox::mem::Reference*);
      void evmLumisection(toolbox::mem::Reference*);
      
      boost::shared_ptr<BU> bu() const { return bu_; }
      boost::shared_ptr<FUproxy> fuProxy() const { return fuProxy_; }
      boost::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
      boost::shared_ptr<DiskWriter> diskWriter() const { return diskWriter_; }
      EventTablePtr eventTable() const { return eventTable_; }
      
      uint32_t runNumber() const { return runNumber_.value_; }
      uint32_t maxEvtsUnderConstruction() const { return maxEvtsUnderConstruction_.value_; }
      
    private:
      
      virtual void do_appendConfigurationItems(InfoSpaceItems&);
      virtual void do_appendMonitoringItems(InfoSpaceItems&);
      
      boost::shared_ptr<BU> bu_;
      boost::shared_ptr<FUproxy> fuProxy_;
      boost::shared_ptr<RUproxy> ruProxy_;
      boost::shared_ptr<DiskWriter> diskWriter_;
      EventTablePtr eventTable_;
      
      xdata::UnsignedInteger32 runNumber_;
      xdata::UnsignedInteger32 maxEvtsUnderConstruction_;
      
    };

  } } //namespace evb::bu

#endif //_evb_bu_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
