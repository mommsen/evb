#ifndef _evb_readoutunit_StateMachine_h_
#define _evb_readoutunit_StateMachine_h_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>

#include "evb/EvBStateMachine.h"
#include "evb/Exception.h"
#include "log4cplus/loggingmacros.h"
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

    ///////////////////////
    // The state machine //
    ///////////////////////

    template<class Owner>
    class StateMachine : public EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >
    {

    public:

      StateMachine(Owner*);

      void mismatchEvent(const MismatchDetected&);

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
  this->reasonForError_ = evt.getTraceback();

  LOG4CPLUS_ERROR(this->getLogger(), this->reasonForError_);

  try
  {
    this->rcmsStateNotifier_.stateChanged("SynchLoss", evt.getReason());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify RCMS about SynchLoss!");
  }

  try
  {
    this->app_->notifyQualified("error", evt.getException());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify the sentinel about SynchLoss!");
  }
}


#endif //_evb_readoutunit_StateMachine_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
