#ifndef _evb_readoutunit_FerolStream_h_
#define _evb_readoutunit_FerolStream_h_

#include <sched.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "cgicc/HTMLClasses.h"
#include "evb/Constants.h"
#include "evb/DumpUtility.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/Exception.h"
#include "evb/OneToOneQueue.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/FedFragmentFactory.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/InputMonitor.h"
#include "evb/readoutunit/StateMachine.h"
#include "evb/readoutunit/SuperFragment.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationStub.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Represent one FEROL data stream
    */

    template<class ReadoutUnit, class Configuration>
    class FerolStream
    {
    public:

      FerolStream(ReadoutUnit*, const uint16_t fedId);

      /**
       * Return the next FED fragement. Return false if none is available
       */
      bool getNextFedFragment(FedFragmentPtr&);

      /**
       * Append the next FED fragment to the super fragment
       */
      virtual void appendFedFragment(SuperFragmentPtr&);

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);

      /**
       * Drain the remainig events
       */
      virtual void drain();

      /**
       * Block the input queue
       */
      virtual void blockInput();

      /**
       * Stop processing events
       */
      virtual void stopProcessing();

      /**
       * Declare this stream as the master stream
       */
      void useAsMaster()
      { isMasterStream_ = true; }

      /**
       * Set the trigger rate for generating events
       */
      virtual void setMaxTriggerRate(const uint32_t maxTriggerRate) {};

      /**
       * Write the next count FED fragments to a text file
       */
      void writeNextFragmentsToFile(const uint16_t count)
      { writeNextFragments_ = count; }

      /**
       * Set the event number at which the generator shall stop
       */
      void setEventNumberToStop(const uint32_t value)
      { eventNumberToStop_ = value; }

      /**
       * Return the EvBid factory for this stream
       */
      EvBidFactoryPtr getEvBidFactory() const
      { return evbIdFactory_; }

      /**
       * Return the FED id associated with this stream
       */
      uint16_t getFedId() const
      { return fedId_; }

      /**
       * Return the requested monitoring quantities.
       */
      void retrieveMonitoringQuantities(uint32_t& fragmentSize,
                                        uint32_t& fragmentSizeStdDev,
                                        uint32_t& queueElements,
                                        uint32_t& corruptedEvents,
                                        uint32_t& eventsOutOfSequence,
                                        uint32_t& crcErrors,
                                        uint32_t& bxErrors);

      /**
       * Return a CGI table row with statistics for this FED
       */
      virtual cgicc::tr getFedTableRow() const;

      /**
       * Return the content of the fragment FIFO as HTML snipped
       */
      virtual cgicc::div getHtmlSnippedForFragmentFIFO() const
      { return fragmentFIFO_.getHtmlSnipped(); }


    protected:

      void addFedFragment(FedFragmentPtr&);
      void maybeDumpFragmentToFile(const FedFragmentPtr&);
      void updateInputMonitor(const FedFragmentPtr&);

      ReadoutUnit* readoutUnit_;
      const uint16_t fedId_;
      bool isMasterStream_;
      volatile bool doProcessing_;
      volatile bool blockInput_;
      volatile bool syncLoss_;
      uint32_t eventNumberToStop_;
      uint32_t bxErrors_;

      struct SocketMonitor
      {
        uint16_t rate;
        uint32_t usedBufferSize;
        uint32_t usedBufferSizeStdDev;
        PerformanceMonitor perf;

        SocketMonitor() { reset(); }

        void reset() { rate=0;usedBufferSize=0;usedBufferSizeStdDev=0; }
      };
      SocketMonitor socketMonitor_;
      mutable boost::mutex socketMonitorMutex_;

      EvBidFactoryPtr evbIdFactory_;
      FedFragmentFactory<ReadoutUnit> fedFragmentFactory_;

      using FragmentFIFO = OneToOneQueue<FedFragmentPtr>;
      FragmentFIFO fragmentFIFO_;

      InputMonitor inputMonitor_;
      mutable boost::mutex inputMonitorMutex_;


    private:

      void resetMonitoringCounters();

      FedFragmentPtr firstFragmentAfterResync_;
      uint16_t writeNextFragments_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////


template<class ReadoutUnit,class Configuration>
evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::FerolStream
(
  ReadoutUnit* readoutUnit,
  const uint16_t fedId
) :
  readoutUnit_(readoutUnit),
  fedId_(fedId),
  isMasterStream_(false),
  doProcessing_(false),
  blockInput_(false),
  syncLoss_(false),
  eventNumberToStop_(0),
  bxErrors_(0),
  evbIdFactory_( new EvBidFactory() ),
  fedFragmentFactory_(readoutUnit,evbIdFactory_),
  fragmentFIFO_(readoutUnit,"fragmentFIFO_FED_"+boost::lexical_cast<std::string>(fedId)),
  writeNextFragments_(0)
{
  fragmentFIFO_.resize(readoutUnit->getConfiguration()->fragmentFIFOCapacity);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragment
(
  FedFragmentPtr& fedFragment
)
{
  if ( ! doProcessing_ ) return;

  updateInputMonitor(fedFragment);
  maybeDumpFragmentToFile(fedFragment);

  fragmentFIFO_.enqWait(fedFragment,doProcessing_);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::updateInputMonitor
(
  const FedFragmentPtr& fragment
)
{
  boost::mutex::scoped_lock sl(inputMonitorMutex_);
  const uint32_t fedSize = fragment->getFedSize();

  ++(inputMonitor_.perf.i2oCount);
  ++(inputMonitor_.perf.logicalCount);
  inputMonitor_.perf.sumOfSizes += fedSize;
  inputMonitor_.perf.sumOfSquares += fedSize*fedSize;
  inputMonitor_.lastEventNumber = fragment->getEventNumber();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::maybeDumpFragmentToFile
(
  const FedFragmentPtr& fragment
)
{
  if ( writeNextFragments_ > 0 )
  {
    fedFragmentFactory_.writeFragmentToFile(fragment,"Requested by user");
    --writeNextFragments_;
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::getNextFedFragment(FedFragmentPtr& fedFragment)
{
  if ( ! doProcessing_ )
    throw exception::HaltRequested();

  if ( blockInput_ ) return false;

  return fragmentFIFO_.deq(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::appendFedFragment(SuperFragmentPtr& superFragment)
{
  FedFragmentPtr fedFragment;

  if ( ! syncLoss_ )
  {
    while ( ! getNextFedFragment(fedFragment) ) { sched_yield(); }

    if ( fedFragment->isOutOfSequence() || fedFragment->isCorrupted() )
    {
      // if we get these here, we tolerated them. Thus just mark them as discarded in the super fragment
      syncLoss_ = true;
      superFragment->discardFedId(fedId_);
      return;
    }
  }
  else
  {
    if ( firstFragmentAfterResync_ )
    {
      // the FED has already resync'd
      if ( superFragment->getEvBid().resynced() )
      {
        // the master has resync'd, too
        fedFragment.swap(firstFragmentAfterResync_);
        syncLoss_ = false;
        readoutUnit_->getStateMachine()->processFSMEvent( Recovered(fedId_) );
      }
      else
      {
        // keep discarding data until the master has resync'd
        superFragment->discardFedId(fedId_);
        return;
      }
    }
    else
    {
      if ( superFragment->getEvBid().resynced() )
      {
        // the master has resync'd. Pull FED data until the FED resync'd, too.
        do
        {
          while ( ! getNextFedFragment(fedFragment) ) { sched_yield(); }
        }
        while ( ! fedFragment->getEvBid().resynced() );
        syncLoss_ = false;
        readoutUnit_->getStateMachine()->processFSMEvent( Recovered(fedId_) );
      }
      else
      {
        // keep discarding the events until we see a resync
        while ( getNextFedFragment(fedFragment) )
        {
          if ( fedFragment->getEvBid().resynced() )
          {
            firstFragmentAfterResync_.swap(fedFragment);
            return;
          }
        }
        superFragment->discardFedId(fedId_);
        return;
      }
    }
  }

  try
  {
    if ( ! superFragment->append(fedFragment) && readoutUnit_->getConfiguration()->checkBxId )
    {
      if ( evb::isFibonacci(++bxErrors_) )
      {
        std::ostringstream msg;
        msg << "Received " << bxErrors_
          << " fragments from FED " << fedFragment->getFedId()
          << " (" << readoutUnit_->getSubSystem() << ")"
          << " with a wrong bunch crossing id."
          << " The current event " << fedFragment->getEvBid().eventNumber()
          << " has bunch crossing id " << fedFragment->getEvBid().bxId()
          << ", but TCDS has BX " << superFragment->getEvBid().bxId();
        LOG4CPLUS_WARN(readoutUnit_->getApplicationLogger(),msg.str());
      }
    }
  }
  catch(exception::MismatchDetected& e)
  {
    fedFragmentFactory_.writeFragmentToFile(fedFragment,e.message());
    syncLoss_ = true;

    if ( readoutUnit_->getConfiguration()->tolerateOutOfSequenceEvents
         && !isMasterStream_ )
    {
      readoutUnit_->getStateMachine()->processFSMEvent( DataLoss(e,fedId_) );
    }
    else
    {
      readoutUnit_->getStateMachine()->processFSMEvent( MismatchDetected(e) );
      throw exception::HaltRequested();
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  resetMonitoringCounters();
  evbIdFactory_->reset(runNumber);
  fedFragmentFactory_.reset(runNumber);
  doProcessing_ = true;
  blockInput_ = false;
  syncLoss_ = false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::drain()
{
  while ( !fragmentFIFO_.empty() ) ::usleep(1000);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::blockInput()
{
  blockInput_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::stopProcessing()
{
  doProcessing_ = false;
  fragmentFIFO_.clear();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(inputMonitorMutex_);
    inputMonitor_.reset();
  }
  {
    boost::mutex::scoped_lock sl(socketMonitorMutex_);
    socketMonitor_.reset();
  }
  bxErrors_ = 0;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::retrieveMonitoringQuantities
(
  uint32_t& fragmentSize,
  uint32_t& fragmentSizeStdDev,
  uint32_t& queueElements,
  uint32_t& corruptedEvents,
  uint32_t& eventsOutOfSequence,
  uint32_t& crcErrors,
  uint32_t& bxErrors
)
{
  {
    boost::mutex::scoped_lock sl(inputMonitorMutex_);

    fragmentSize = inputMonitor_.eventSize;
    fragmentSizeStdDev = inputMonitor_.eventSizeStdDev;
    const double deltaT = inputMonitor_.perf.deltaT();
    inputMonitor_.rate = inputMonitor_.perf.logicalRate(deltaT);
    inputMonitor_.throughput = inputMonitor_.perf.throughput(deltaT);
    if ( inputMonitor_.rate > 0 )
    {
      inputMonitor_.eventSize = inputMonitor_.perf.size();
      inputMonitor_.eventSizeStdDev = inputMonitor_.perf.sizeStdDev();
    }
    inputMonitor_.perf.reset();
  }

  {
    boost::mutex::scoped_lock sl(socketMonitorMutex_);

    const double deltaT = socketMonitor_.perf.deltaT();
    socketMonitor_.rate = socketMonitor_.perf.logicalRate(deltaT);
    if ( socketMonitor_.rate > 0 )
    {
      socketMonitor_.usedBufferSize = socketMonitor_.perf.size();
      socketMonitor_.usedBufferSizeStdDev = socketMonitor_.perf.sizeStdDev();
    }
    socketMonitor_.perf.reset();
  }

  queueElements = fragmentFIFO_.elements();

  corruptedEvents = fedFragmentFactory_.getCorruptedEvents();
  eventsOutOfSequence = fedFragmentFactory_.getEventsOutOfSequence();
  crcErrors = fedFragmentFactory_.getCRCerrors();
  bxErrors = bxErrors_;
}


template<class ReadoutUnit,class Configuration>
cgicc::tr evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::getFedTableRow() const
{
  using namespace cgicc;
  const std::string fedId = boost::lexical_cast<std::string>(fedId_);

  tr row;
  if ( isMasterStream_ )
    row.add(td(fedId+"*"));
  else
    row.add(td(fedId));
  row.add(td()
          .add(button("dump").set("type","button").set("title","write the next FED fragment to /tmp")
               .set("onclick","dumpFragments("+fedId+",1);")));
  {
    boost::mutex::scoped_lock sl(inputMonitorMutex_);

    row.add(td(boost::lexical_cast<std::string>(inputMonitor_.lastEventNumber)));
    row.add(td(boost::lexical_cast<std::string>(static_cast<uint32_t>(inputMonitor_.eventSize))
               +" +/- "+boost::lexical_cast<std::string>(static_cast<uint32_t>(inputMonitor_.eventSizeStdDev))));
    row.add(td(doubleToString(inputMonitor_.throughput / 1e6,1)));
  }

  row.add(td(boost::lexical_cast<std::string>(fedFragmentFactory_.getCRCerrors())))
    .add(td(boost::lexical_cast<std::string>(fedFragmentFactory_.getCorruptedEvents())))
    .add(td(boost::lexical_cast<std::string>(fedFragmentFactory_.getEventsOutOfSequence())))
    .add(td(boost::lexical_cast<std::string>(bxErrors_)));

  {
    boost::mutex::scoped_lock sl(socketMonitorMutex_);

    row.add(td(boost::lexical_cast<std::string>(socketMonitor_.usedBufferSize)
               +" +/- "+boost::lexical_cast<std::string>(socketMonitor_.usedBufferSizeStdDev)));
    row.add(td(boost::lexical_cast<std::string>(socketMonitor_.rate)));
  }

  return row;
}


#endif // _evb_readoutunit_FerolStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
