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
    // The state machine //
    ///////////////////////
    
    typedef EvBStateMachine<StateMachine,bu::Outermost> EvBStateMachine;
    class StateMachine: public EvBStateMachine
    {
      
    public:
      
      StateMachine
      (
        BU*,
        boost::shared_ptr<RUproxy>,
        boost::shared_ptr<DiskWriter>,
        EventTablePtr
      );
      
      void buCache(toolbox::mem::Reference*);
      
      BU* bu() const { return bu_; }
      boost::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
      boost::shared_ptr<DiskWriter> diskWriter() const { return diskWriter_; }
      EventTablePtr eventTable() const { return eventTable_; }
      
      uint32_t runNumber() const { return runNumber_.value_; }
      uint32_t maxEvtsUnderConstruction() const { return maxEvtsUnderConstruction_.value_; }
      
    private:
      
      virtual void do_appendConfigurationItems(InfoSpaceItems&);
      virtual void do_appendMonitoringItems(InfoSpaceItems&);
      
      BU* bu_;
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
