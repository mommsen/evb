#ifndef _evb_readoutunit_FerolStream_h_
#define _evb_readoutunit_FerolStream_h_

#include <sched.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "cgicc/HTMLClasses.h"
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
       * Add a FED fragment received from the input stream
       */
      void addFedFragment(toolbox::mem::Reference*, tcpla::MemoryCache*);

      /**
       * Return the next FED fragement. Return false if none is available
       */
      bool getNextFedFragment(FedFragmentPtr&);

      /**
       * Append the next FED fragment to the super fragment
       */
      virtual void appendFedFragment(SuperFragmentPtr&);

      /**
       * Define the function to be used to extract the lumi section from the payload
       * If the function is null, fake lumi sections with fakeLumiSectionDuration are generated
       */
      void setLumiSectionFunction(boost::function< uint32_t(const FedFragment::DataLocations&) >& lumiSectionFunction);

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);

      /**
       * Drain the remainig events
       */
      virtual void drain();

      /**
       * Stop processing events
       */
      virtual void stopProcessing();

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
       * Return the requested monitoring quantities.
       */
      void retrieveMonitoringQuantities(uint32_t& dataReadyCount,
                                        uint32_t& queueElements,
                                        uint32_t& corruptedEvents,
                                        uint32_t& crcErrors);

      /**
       * Return a CGI table row with statistics for this FED
       */
      cgicc::tr getFedTableRow(const bool isMasterStream) const;

      /**
       * Return the content of the fragment FIFO as HTML snipped
       */
      cgicc::div getHtmlSnippedForFragmentFIFO() const
      { return fragmentFIFO_.getHtmlSnipped(); }


    protected:

      void addFedFragment(FedFragmentPtr&);
      void addFedFragmentWithEvBid(FedFragmentPtr&);
      void maybeDumpFragmentToFile(const FedFragmentPtr&);
      void updateInputMonitor(const FedFragmentPtr&);

      ReadoutUnit* readoutUnit_;
      const uint16_t fedId_;
      const boost::shared_ptr<Configuration> configuration_;
      volatile bool doProcessing_;
      volatile bool syncLoss_;
      uint32_t eventNumberToStop_;

      EvBidFactory evbIdFactory_;
      FedFragmentFactory<ReadoutUnit> fedFragmentFactory_;

      typedef OneToOneQueue<FedFragmentPtr> FragmentFIFO;
      FragmentFIFO fragmentFIFO_;


    private:

      EvBid getEvBid(const FedFragmentPtr&);
      void resetMonitoringCounters();

      boost::function< uint32_t(const FedFragment::DataLocations&) > lumiSectionFunction_;
      uint16_t writeNextFragments_;

      InputMonitor inputMonitor_;
      mutable boost::mutex inputMonitorMutex_;

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
  configuration_(readoutUnit->getConfiguration()),
  doProcessing_(false),
  syncLoss_(false),
  eventNumberToStop_(0),
  fedFragmentFactory_(readoutUnit),
  fragmentFIFO_(readoutUnit,"fragmentFIFO_FED_"+boost::lexical_cast<std::string>(fedId)),
  writeNextFragments_(0)
{
  fragmentFIFO_.resize(configuration_->fragmentFIFOCapacity);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragment
(
  toolbox::mem::Reference* bufRef,
  tcpla::MemoryCache* cache
)
{
  FedFragmentPtr fedFragment = fedFragmentFactory_.getFedFragment(bufRef,cache);
  addFedFragment(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragment
(
  FedFragmentPtr& fedFragment
)
{
  try
  {
    const EvBid evbId = getEvBid(fedFragment);
    fedFragment->setEvBid(evbId);

    addFedFragmentWithEvBid(fedFragment);
  }
  catch(exception::TCDS& e)
  {
    std::ostringstream oss;
    oss << "Failed to extract lumi section from FED " << fedId_;

    XCEPT_DECLARE_NESTED(exception::TCDS, sentinelException,
                         oss.str(),e);
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(exception::EventOutOfSequence& e)
  {
    syncLoss_ = true;
    fedFragmentFactory_.writeFragmentToFile(fedFragment,e.message());

    std::ostringstream oss;
    oss << "Received an event out of sequence from FED " << fedId_;

    XCEPT_DECLARE_NESTED(exception::EventOutOfSequence, sentinelException,
                         oss.str(),e);
    readoutUnit_->getStateMachine()->processFSMEvent( EventOutOfSequence(sentinelException) );
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragmentWithEvBid
(
  FedFragmentPtr& fedFragment
)
{
  if ( ! doProcessing_ ) return;

  updateInputMonitor(fedFragment);
  maybeDumpFragmentToFile(fedFragment);

  fragmentFIFO_.enqWait(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::setLumiSectionFunction
(
  boost::function< uint32_t(const FedFragment::DataLocations&) >& lumiSectionFunction
)
{
  if ( lumiSectionFunction )
    lumiSectionFunction_ = lumiSectionFunction;
  else
    evbIdFactory_.setFakeLumiSectionDuration(configuration_->fakeLumiSectionDuration);
}


template<class ReadoutUnit,class Configuration>
evb::EvBid evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::getEvBid(const FedFragmentPtr& fedFragment)
{
    return evbIdFactory_.getEvBid(fedFragment->getEventNumber());
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
  if ( ! doProcessing_ || syncLoss_ )
    throw exception::HaltRequested();

  return fragmentFIFO_.deq(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::appendFedFragment(SuperFragmentPtr& superFragment)
{
  FedFragmentPtr fedFragment;
  while ( ! getNextFedFragment(fedFragment) ) { sched_yield(); }

  try
  {
    superFragment->append(fedFragment);
  }
  catch(exception::MismatchDetected& e)
  {
    fedFragmentFactory_.writeFragmentToFile(fedFragment,e.message());
    syncLoss_ = true;
    readoutUnit_->getStateMachine()->processFSMEvent( MismatchDetected(e) );
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  resetMonitoringCounters();
  evbIdFactory_.reset(runNumber);
  fedFragmentFactory_.reset(runNumber);
  doProcessing_ = true;
  syncLoss_ = false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::drain()
{
  while ( !fragmentFIFO_.empty() ) ::usleep(1000);
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
  inputMonitor_.reset();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::retrieveMonitoringQuantities
(
  uint32_t& dataReadyCount,
  uint32_t& queueElements,
  uint32_t& corruptedEvents,
  uint32_t& crcErrors
)
{
  {
    boost::mutex::scoped_lock sl(inputMonitorMutex_);

    dataReadyCount += inputMonitor_.perf.logicalCount;

    inputMonitor_.rate = inputMonitor_.perf.logicalRate();
    inputMonitor_.bandwidth = inputMonitor_.perf.bandwidth();
    const uint32_t eventSize = inputMonitor_.perf.size();
    if ( eventSize > 0 )
    {
      inputMonitor_.eventSize = eventSize;
      inputMonitor_.eventSizeStdDev = inputMonitor_.perf.sizeStdDev();
    }
    inputMonitor_.perf.reset();
  }

  queueElements = fragmentFIFO_.elements();

  corruptedEvents = fedFragmentFactory_.getCorruptedEvents();
  crcErrors = fedFragmentFactory_.getCRCerrors();
}


template<class ReadoutUnit,class Configuration>
cgicc::tr evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::getFedTableRow(const bool isMasterStream) const
{
  using namespace cgicc;
  const std::string fedId = boost::lexical_cast<std::string>(fedId_);

  tr row;
  if (isMasterStream)
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

    std::ostringstream str;
    str.setf(std::ios::fixed);
    str.precision(1);
    str << inputMonitor_.bandwidth / 1e6;
    row.add(td(str.str()));
  }

  row.add(td(boost::lexical_cast<std::string>(fedFragmentFactory_.getCRCerrors())))
    .add(td(boost::lexical_cast<std::string>(fedFragmentFactory_.getCorruptedEvents())));

  return row;
}


#endif // _evb_readoutunit_FerolStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
