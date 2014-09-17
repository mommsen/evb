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
      mutable exception::MismatchDetected exception_;
    };

    class EventOutOfSequence : public boost::statechart::event<EventOutOfSequence>
    {
    public:
      EventOutOfSequence(exception::EventOutOfSequence& exception) : exception_(exception) {};
      std::string getReason() const { return exception_.message(); }
      std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
      xcept::Exception& getException() const { return exception_; }

    private:
      mutable exception::EventOutOfSequence exception_;
    };


    ///////////////////////
    // The state machine //
    ///////////////////////

    template<class Owner>
    class StateMachine : public EvBStateMachine< StateMachine<Owner>,Outermost<Owner> >
    {

    public:

      StateMachine(Owner*);

      // Need to explicitly define the functions, as the standard says:
      // derived-to-base conversion (4.10) are not applied in function template evaluation
      void mismatchDetected(const MismatchDetected& evt);
      void eventOutOfSequence(const EventOutOfSequence& evt);

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
void evb::readoutunit::StateMachine<Owner>::mismatchDetected(const MismatchDetected& evt)
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


template<class Owner>
void evb::readoutunit::StateMachine<Owner>::eventOutOfSequence(const EventOutOfSequence& evt)
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
