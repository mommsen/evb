#ifndef _evb_ru_StateMachine_h_
#define _evb_ru_StateMachine_h_

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/Exception.h"
#include "evb/EvBStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace evb {

  class RU;
  
  namespace ru {
    
    class BUproxy;
    class Input;
    class StateMachine;
    class Outermost;
    
    ///////////////////////
    // Transition events //
    ///////////////////////
    
    class MismatchDetected : public boost::statechart::event<MismatchDetected>
    {
    public:
      MismatchDetected(exception::MismatchDetected& exception) : exception_(exception) {};
      std::string getReason() const { return exception_.message(); }
      std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
      xcept::Exception& getException() const { return exception_; }
      
    private:
      mutable xcept::Exception exception_;
    };
    
    class TimedOut : public boost::statechart::event<TimedOut>
    {
    public:
      TimedOut(exception::TimedOut& exception) : exception_(exception) {};
      std::string getReason() const { return exception_.message(); }
      std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
      xcept::Exception& getException() const { return exception_; }
      
    private:
      mutable xcept::Exception exception_;
    };
    
    ///////////////////////
    // The state machine //
    ///////////////////////
    
    typedef EvBStateMachine<StateMachine,ru::Outermost> EvBStateMachine;
    class StateMachine: public EvBStateMachine
    {
      
    public:
      
      StateMachine
      (
        RU*,
        boost::shared_ptr<Input>,
        boost::shared_ptr<BUproxy>
      );
      
      void mismatchEvent(const MismatchDetected&);
      void timedOutEvent(const TimedOut&);
      
      RU* ru() const { return ru_; }
      boost::shared_ptr<Input> ruInput() const { return ruInput_; }
      boost::shared_ptr<BUproxy> buProxy() const { return buProxy_; }
      
      uint32_t runNumber() const { return runNumber_.value_; }
      
    private:
      
      virtual void do_appendConfigurationItems(InfoSpaceItems&);
      virtual void do_appendMonitoringItems(InfoSpaceItems&);
      
      RU* ru_;
      boost::shared_ptr<Input> ruInput_;
      boost::shared_ptr<BUproxy> buProxy_;

      xdata::UnsignedInteger32 runNumber_;
      
    };
    
  } } //namespace evb::ru

#endif //_evb_ru_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
