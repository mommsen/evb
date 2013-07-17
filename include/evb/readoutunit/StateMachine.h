#ifndef _evb_readoutunit_StateMachine_h_
#define _evb_readoutunit_StateMachine_h_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/EvBStateMachine.h"
#include "evb/Exception.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace evb {

  namespace readoutunit {

    template<class> class Outermost;

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

    template<class Owner>
    class StateMachine : public EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >
    {

    public:

      StateMachine(Owner*);

      void mismatchEvent(const MismatchDetected&);
      void timedOutEvent(const TimedOut&);

      Owner* getOwner() const
      { return owner_; }

    private:

      Owner* owner_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Owner>
evb::readoutunit::StateMachine<Owner>::StateMachine(Owner* owner) :
EvBStateMachine< StateMachine<Owner>,readoutunit::Outermost<Owner> >(owner),
owner_(owner)
{}


template<class Owner>
void evb::readoutunit::StateMachine<Owner>::mismatchEvent(const MismatchDetected& evt)
{
  LOG4CPLUS_ERROR(this->getLogger(), evt.getTraceback());

  this->rcmsStateNotifier_.stateChanged("MismatchDetectedBackPressuring", evt.getReason());

  this->app_->notifyQualified("error", evt.getException());
}


template<class Owner>
void evb::readoutunit::StateMachine<Owner>::timedOutEvent(const TimedOut& evt)
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
