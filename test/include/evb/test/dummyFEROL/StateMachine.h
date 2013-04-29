#ifndef _evb_test_dummyFEROL_StateMachine_h_
#define _evb_test_dummyFEROL_StateMachine_h_

#include <boost/statechart/event_base.hpp>

#include "evb/Exception.h"
#include "evb/EvBStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace evb {
  namespace test {
    
    class DummyFEROL;
    
    namespace dummyFEROL {
      
      class StateMachine;
      class Outermost;
      
      ///////////////////////
      // The state machine //
      ///////////////////////
      
      typedef EvBStateMachine<StateMachine,Outermost> EvBStateMachine;
      class StateMachine: public EvBStateMachine
      {
        
      public:
        
        StateMachine(DummyFEROL*);
        
        DummyFEROL* dummyFEROL() const { return dummyFEROL_; }

      private:
        
        DummyFEROL* dummyFEROL_;
        
      };
      
    } } } //namespace evb::test::dummyFEROL

#endif //_evb_test_dummyFEROL_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
