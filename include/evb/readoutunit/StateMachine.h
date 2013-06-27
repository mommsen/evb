#ifndef _evb_readoutunit_StateMachine_h_
#define _evb_readoutunit_StateMachine_h_

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/EvBStateMachine.h"
#include "evb/Exception.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace evb {

  namespace readoutunit {
    

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
    
    //class StateMachine;
    class Outermost;
    // class Configuration;
    // template<class Configuration> class Input;
    // template<class Configuration,class Input> class ReadoutUnit;
    //template<class ReadoutUnit> class StateMachine;    
    
    // typedef StateMachine< ReadoutUnit< Configuration,Input<Configuration> > > ReadoutUnitStateMachine;
    // typedef EvBStateMachine<ReadoutUnitStateMachine,Outermost> EvBStateMachine;
    //typedef EvBStateMachine<StateMachine<ReadoutUnit>,Outermost> EvBStateMachine;
    template<class ReadoutUnit>
    class StateMachine : public EvBStateMachine<StateMachine<ReadoutUnit>,Outermost>
    {
      
    public:
      
      StateMachine(ReadoutUnit*);
      
      void mismatchEvent(const MismatchDetected&);
      void timedOutEvent(const TimedOut&);
      
      ReadoutUnit* getReadoutUnit() const
      { return readoutUnit_; }
      
    private:
      
      ReadoutUnit* readoutUnit_;

    };
    
  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit>
evb::readoutunit::StateMachine<ReadoutUnit>::StateMachine(ReadoutUnit* readoutUnit) :
EvBStateMachine<StateMachine<ReadoutUnit>,readoutunit::Outermost>(readoutUnit),
readoutUnit_(readoutUnit)
{}


template<class ReadoutUnit>
void evb::readoutunit::StateMachine<ReadoutUnit>::mismatchEvent(const MismatchDetected& evt)
{
  LOG4CPLUS_ERROR(this->getLogger(), evt.getTraceback());
  
  this->rcmsStateNotifier_.stateChanged("MismatchDetectedBackPressuring", evt.getReason());

  this->app_->notifyQualified("error", evt.getException());
}


template<class ReadoutUnit>
void evb::readoutunit::StateMachine<ReadoutUnit>::timedOutEvent(const TimedOut& evt)
{
  LOG4CPLUS_ERROR(this->getLogger(), evt.getTraceback());
  
  this->rcmsStateNotifier_.stateChanged("TimedOutBackPressuring", evt.getReason());

  this->app_->notifyQualified("error", evt.getException());
}


#endif //_evb_readoutunit_StateMachine_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
