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
    
    class EvmRuDataReady : public boost::statechart::event<EvmRuDataReady> 
    {
    public:
      EvmRuDataReady(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getDataReadyMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
    };
    
    class RuSend : public boost::statechart::event<RuSend> 
    {
    public:
      RuSend(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};
      
      toolbox::mem::Reference* getSendMsg() const { return bufRef_; };
      
    private:
      toolbox::mem::Reference* bufRef_;
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
        boost::shared_ptr<RU>,
        boost::shared_ptr<Input>,
        boost::shared_ptr<BUproxy>
      );
      
      void mismatchEvent(const MismatchDetected&);
      void timedOutEvent(const TimedOut&);
      
      void ruReadout(toolbox::mem::Reference*);
      void evmRuDataReady(toolbox::mem::Reference*);
      void ruSend(toolbox::mem::Reference*);
      
      boost::shared_ptr<BUproxy> buProxy() const { return buProxy_; }
      boost::shared_ptr<RU> ru() const { return ru_; }
      boost::shared_ptr<Input> ruInput() const { return ruInput_; }
      
    private:
      
      boost::shared_ptr<RU> ru_;
      boost::shared_ptr<Input> ruInput_;
      boost::shared_ptr<BUproxy> buProxy_;
      
    };
    
  } } //namespace evb::ru

#endif //_evb_ru_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
