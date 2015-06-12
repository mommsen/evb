#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/Exception.h"
#include "xcept/tools.h"

#include <boost/bind.hpp>

evb::EvBidFactory::EvBidFactory() :
  runNumber_(0),
  previousEventNumber_(0),
  resyncCount_(0),
  fakeLumiSectionDuration_(0),
  fakeLumiSection_(1)
{}


evb::EvBidFactory::~EvBidFactory()
{
  stopFakeLumiThread();
}


void evb::EvBidFactory::reset(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  previousEventNumber_ = 0;
  resyncCount_ = 0;

  stopFakeLumiThread();
  fakeLumiSection_ = 1;
  if ( fakeLumiSectionDuration_.total_seconds() > 0 )
  {
    fakeLumiThread_.reset(
      new boost::thread( boost::bind( &evb::EvBidFactory::fakeLumiActivity, this) )
    );
  }
}


void evb::EvBidFactory::stopFakeLumiThread()
{
  if ( fakeLumiThread_.get() )
  {
    fakeLumiThread_->interrupt();
    fakeLumiThread_->join();
  }
}


void evb::EvBidFactory::fakeLumiActivity()
{
  boost::system_time nextLumiSectionStartTime = boost::get_system_time();
  while(1)
  {
    nextLumiSectionStartTime += fakeLumiSectionDuration_;
    boost::this_thread::sleep(nextLumiSectionStartTime);
    ++fakeLumiSection_;
  }
}


evb::EvBid evb::EvBidFactory::getEvBid()
{
  const uint32_t fakeEventNumber = (previousEventNumber_+1) % (1 << 24);
  return getEvBid(fakeEventNumber);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  return getEvBid(eventNumber,fakeLumiSection_);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber, const uint32_t lumiSection)
{
  if ( eventNumber != previousEventNumber_ + 1 )
  {
    if ( (eventNumber == 1 && previousEventNumber_ > 0) ||
         (eventNumber == 0 && previousEventNumber_ == 16777215 ) ) // (2^24)-1
    {
      ++resyncCount_;
    }
    else
    {
      std::ostringstream msg;
      if ( eventNumber == previousEventNumber_ )
      {
        msg << "Received a duplicate event number " << eventNumber;
      }
      else if ( eventNumber > 1 && previousEventNumber_ == 0 && resyncCount_ == 0 )
      {
        msg << "Received " << eventNumber << " as first event number (should be 1.) Have the buffers not be drained?";
      }
      else
      {
        msg << "Skipped from event number " << previousEventNumber_;
        msg << " to " << eventNumber;
      }

      previousEventNumber_ = eventNumber;
      XCEPT_RAISE(exception::EventOutOfSequence,msg.str());
    }
  }

  previousEventNumber_ = eventNumber;

  return EvBid(resyncCount_,eventNumber,lumiSection,runNumber_);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
