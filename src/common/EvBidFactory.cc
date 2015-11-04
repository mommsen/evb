#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/Exception.h"
#include "xcept/tools.h"

#include <boost/bind.hpp>

evb::EvBidFactory::EvBidFactory() :
  runNumber_(0),
  previousEventNumber_(0),
  resyncCount_(0),
  resyncAtEventNumber_(1<<25),
  fakeLumiSectionDuration_(0),
  fakeLumiSection_(0),
  doFakeLumiSections_(false)
{}


evb::EvBidFactory::~EvBidFactory()
{
  stopFakeLumiThread();
}


void evb::EvBidFactory::setLumiSectionFunction(LumiSectionFunction& lumiSectionFunction)
{
  lumiSectionFunction_ = lumiSectionFunction;
}


void evb::EvBidFactory::setFakeLumiSectionDuration(const uint32_t duration)
{
  fakeLumiSectionDuration_ = boost::posix_time::seconds(duration);
}


void evb::EvBidFactory::reset(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  previousEventNumber_ = 0;
  resyncCount_ = 0;

  stopFakeLumiThread();
  if ( fakeLumiSectionDuration_.total_seconds() > 0 )
  {
    fakeLumiSection_ = 1;
    doFakeLumiSections_ = true;
    fakeLumiThread_.reset(
      new boost::thread( boost::bind( &evb::EvBidFactory::fakeLumiActivity, this) )
    );
  }
  else
  {
    fakeLumiSection_ = 0;
  }
}


void evb::EvBidFactory::stopFakeLumiThread()
{
  if ( fakeLumiThread_.get() )
  {
    doFakeLumiSections_ = false;
    fakeLumiThread_->interrupt();
    fakeLumiThread_->join();
  }
}


void evb::EvBidFactory::fakeLumiActivity()
{
  boost::system_time nextLumiSectionStartTime = boost::get_system_time();
  while(doFakeLumiSections_)
  {
    nextLumiSectionStartTime += fakeLumiSectionDuration_;
    boost::this_thread::sleep(nextLumiSectionStartTime);
    ++fakeLumiSection_;
  }
}


void evb::EvBidFactory::resyncAtEvent(const uint32_t eventNumber)
{
  resyncAtEventNumber_ = eventNumber;
}


evb::EvBid evb::EvBidFactory::getEvBid()
{
  if ( previousEventNumber_ >= resyncAtEventNumber_ )
  {
    resyncAtEventNumber_ = 1 << 25;
    return getEvBid(1,1);
  }
  else
  {
    const uint32_t fakeEventNumber = (previousEventNumber_+1) % (1 << 24);
    return getEvBid(fakeEventNumber,fakeEventNumber%0xfff);
  }
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber, const uint16_t bxId)
{
  return getEvBid(eventNumber,bxId,fakeLumiSection_);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber, const uint16_t bxId, const uint32_t lumiSection)
{
  bool resynced = false;

  if ( eventNumber != previousEventNumber_ + 1 )
  {
    if (eventNumber == 1 && previousEventNumber_ > 0)
    {
      // A proper TTS resync
      ++resyncCount_;
      resynced = true;
    }
    else if (eventNumber == 0 && previousEventNumber_ == 16777215 ) // (2^24)-1
    {
      // Trigger counter rolled over. Increase our resyncCount nevertheless to assure a unique EvBid.
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

  return EvBid(resynced,resyncCount_,eventNumber,bxId,lumiSection,runNumber_);
}


evb::EvBid evb::EvBidFactory::getEvBid(const uint32_t eventNumber, const uint16_t bxId, const DataLocations& dataLocations)
{
  if ( lumiSectionFunction_ )
  {
    const uint32_t lsNumber = lumiSectionFunction_(dataLocations);
    return getEvBid(eventNumber,bxId,lsNumber);
  }
  else
  {
    return getEvBid(eventNumber,bxId);
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
