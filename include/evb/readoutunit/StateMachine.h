#ifndef _evb_readoutunit_StateMachine_h_
#define _evb_readoutunit_StateMachine_h_

#include <set>

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

    class DataLoss : public boost::statechart::event<DataLoss>
    {
    public:
      DataLoss(xcept::Exception& exception, const uint16_t fedId)
        : exception_(exception), fedId_(fedId) {};
      std::string getReason() const { return exception_.message(); }
      std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
      xcept::Exception& getException() const { return exception_; }
      uint16_t getFedId() const { return fedId_; }

    private:
      mutable xcept::Exception exception_;
      const uint16_t fedId_;
    };

    class Recovered: public boost::statechart::event<Recovered>
    {
    public:
      Recovered(const uint16_t fedId) : fedId_(fedId) {};
      uint16_t getFedId() const { return fedId_; }

    private:
      const uint16_t fedId_;
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
      void dataLoss(const DataLoss& evt);

      Owner* getOwner() const { return owner_; }
      bool hasMissingFeds() const { return !missingFeds_.empty(); }
      bool removeMissingFed(const uint16_t fedId);
      void resetMissingFeds() { missingFeds_.clear(); }

    private:

      Owner* owner_;
      typedef std::set<uint16_t> MissingFeds;
      MissingFeds missingFeds_;

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
  LOG4CPLUS_ERROR(this->getLogger(), "SyncLoss: " << this->reasonForError_);

  try
  {
    this->rcmsStateNotifier_.stateChanged("SyncLoss", evt.getReason());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify RCMS about SyncLoss!");
  }

  try
  {
    this->app_->notifyQualified("error", evt.getException());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify the sentinel about SyncLoss!");
  }
}


template<class Owner>
void evb::readoutunit::StateMachine<Owner>::eventOutOfSequence(const EventOutOfSequence& evt)
{
  this->reasonForError_ = evt.getTraceback();
  LOG4CPLUS_ERROR(this->getLogger(), "SyncLoss: " << this->reasonForError_);

  try
  {
    this->rcmsStateNotifier_.stateChanged("SyncLoss", evt.getReason());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify RCMS about SyncLoss!");
  }

  try
  {
    this->app_->notifyQualified("error", evt.getException());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify the sentinel about SyncLoss!");
  }
}


template<class Owner>
void evb::readoutunit::StateMachine<Owner>::dataLoss(const DataLoss& evt)
{
  this->reasonForError_ = evt.getTraceback();
  LOG4CPLUS_ERROR(this->getLogger(), "MissingData: " << this->reasonForError_);
  missingFeds_.insert( evt.getFedId() );

  try
  {
    this->rcmsStateNotifier_.stateChanged("MissingData", evt.getReason());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify RCMS about SyncLoss!");
  }

  try
  {
    this->app_->notifyQualified("error", evt.getException());
  }
  catch(...) // Catch anything to make sure that we end in a well defined state
  {
    LOG4CPLUS_FATAL(this->getLogger(),"Failed to notify the sentinel about SyncLoss!");
  }
}


template<class Owner>
bool evb::readoutunit::StateMachine<Owner>::removeMissingFed(const uint16_t fedId)
{
  missingFeds_.erase(fedId);
  return missingFeds_.empty();
}


#endif //_evb_readoutunit_StateMachine_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
