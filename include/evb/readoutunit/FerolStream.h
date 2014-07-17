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
#include "evb/FedFragment.h"
#include "evb/OneToOneQueue.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/InputMonitor.h"
#include "evb/readoutunit/StateMachine.h"
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
      void addFedFragment(FedFragmentPtr&);

      /**
       * Return the next FED fragement. Return false if none is available
       */
      bool getNextFedFragment(FedFragmentPtr&);

      /**
       * Append the next FED fragment to the super fragment
       */
      virtual void appendFedFragment(FragmentChainPtr&);

      /**
       * Define the function to be used to extract the lumi section from the payload
       * If the function is null, fake lumi sections with fakeLumiSectionDuration are generated
       */
      void setLumiSectionFunction(boost::function< uint32_t(const unsigned char*) >& lumiSectionFunction);

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
       * Write the next count FED fragments to a text file
       */
      void writeNextFragmentsToFile(const uint16_t count)
      { writeNextFragments_ = count; }

      /**
       * Return the requested monitoring quantities.
       */
      void retrieveMonitoringQuantities(const double deltaT,
                                        uint32_t& dataReadyCount,
                                        uint32_t& queueElements,
                                        uint32_t& eventsWithDataCorruption,
                                        uint32_t& eventsWithCRCerrors);

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

      void addFedFragmentWithEvBid(FedFragmentPtr&);
      void maybeDumpFragmentToFile(const FedFragmentPtr&);
      void updateInputMonitor(const FedFragmentPtr&,const uint32_t fedSize);

      ReadoutUnit* readoutUnit_;
      const uint16_t fedId_;
      const boost::shared_ptr<Configuration> configuration_;
      volatile bool doProcessing_;

      EvBidFactory evbIdFactory_;

      typedef OneToOneQueue<FedFragmentPtr> FragmentFIFO;
      FragmentFIFO fragmentFIFO_;


    private:

      EvBid getEvBid(const FedFragmentPtr&);
      void writeFragmentToFile(const FedFragmentPtr&,const std::string& reasonFordump) const;
      void resetMonitoringCounters();

      boost::function< uint32_t(const unsigned char*) > lumiSectionFunction_;
      uint16_t writeNextFragments_;
      uint32_t runNumber_;

      InputMonitor inputMonitor_;
      mutable boost::mutex inputMonitorMutex_;

      struct FedErrors
      {
        uint32_t corruptedEvents;
        uint32_t crcErrors;

        FedErrors() { reset(); }
        void reset()
        { corruptedEvents=0;crcErrors=0; }
      };
      FedErrors fedErrors_;
      FedErrors previousFedErrors_;
      mutable boost::mutex fedErrorsMutex_;

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
  fragmentFIFO_(readoutUnit,"fragmentFIFO_FED_"+boost::lexical_cast<std::string>(fedId)),
  writeNextFragments_(0),
  runNumber_(0)
{
  fragmentFIFO_.resize(configuration_->fragmentFIFOCapacity);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragment
(
  FedFragmentPtr& fedFragment
)
{
  const EvBid evbId = getEvBid(fedFragment);
  fedFragment->setEvBid(evbId);
  addFedFragmentWithEvBid(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::addFedFragmentWithEvBid
(
  FedFragmentPtr& fedFragment
)
{
  uint32_t fedSize = 0;

  try
  {
    fedSize = fedFragment->checkIntegrity(configuration_->checkCRC);
  }
  catch(exception::DataCorruption& e)
  {
    uint32_t count = 0;
    {
      boost::mutex::scoped_lock sl(fedErrorsMutex_);
      count = ++(fedErrors_.corruptedEvents);
    }

    if ( count <= configuration_->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());

    if ( configuration_->tolerateCorruptedEvents )
    {
      // fragment.reset( new FedFragment(evbId,0,0) );

      LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                      xcept::stdformat_exception_history(e));
      readoutUnit_->notifyQualified("error",e);
    }
    else
    {
      readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
    }
  }
  catch(exception::CRCerror& e)
  {
    uint32_t count = 0;
    {
      boost::mutex::scoped_lock sl(fedErrorsMutex_);
      count = ++(fedErrors_.crcErrors);
    }

    if ( count <= configuration_->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());

    LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("error",e);
  }

  updateInputMonitor(fedFragment,fedSize);
  maybeDumpFragmentToFile(fedFragment);

  if ( doProcessing_ && !configuration_->dropInputData )
    fragmentFIFO_.enqWait(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::setLumiSectionFunction
(
  boost::function< uint32_t(const unsigned char*) >& lumiSectionFunction
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
  const FedFragmentPtr& fragment,
  uint32_t fedSize
)
{
  boost::mutex::scoped_lock sl(inputMonitorMutex_);

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
    writeFragmentToFile(fragment,"Requested by user");
    --writeNextFragments_;
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::writeFragmentToFile
(
  const FedFragmentPtr& fragment,
  const std::string& reasonForDump
) const
{
  std::ostringstream fileName;
  fileName << "/tmp/dump_run" << std::setfill('0') << std::setw(6) << runNumber_
    << "_event" << std::setw(8) << fragment->getEventNumber()
    << "_fed" << std::setw(4) << fragment->getFedId()
    << ".txt";
  std::ofstream dumpFile;
  dumpFile.open(fileName.str().c_str());
  DumpUtility::dump(dumpFile, reasonForDump, fragment->getBufRef());
  dumpFile.close();
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::getNextFedFragment(FedFragmentPtr& fedFragment)
{
  if ( ! doProcessing_ )
    throw exception::HaltRequested();

  return fragmentFIFO_.deq(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::appendFedFragment(FragmentChainPtr& superFragment)
{
  FedFragmentPtr fedFragment;
  while ( ! getNextFedFragment(fedFragment) ) { sched_yield(); }

  superFragment->append(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  resetMonitoringCounters();
  evbIdFactory_.reset(runNumber);
  doProcessing_ = true;
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
  fedErrors_.reset();
  previousFedErrors_.reset();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::FerolStream<ReadoutUnit,Configuration>::retrieveMonitoringQuantities
(
  const double deltaT,
  uint32_t& dataReadyCount,
  uint32_t& queueElements,
  uint32_t& eventsWithDataCorruption,
  uint32_t& eventsWithCRCerrors
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

  {
    boost::mutex::scoped_lock sl(fedErrorsMutex_);

    if ( deltaT > 0 )
    {
      const uint32_t deltaN = fedErrors_.crcErrors - previousFedErrors_.crcErrors;
      const double rate = deltaN / deltaT;
      if ( rate > configuration_->maxCRCErrorRate )
      {
        std::ostringstream msg;
        msg.setf(std::ios::fixed);
        msg.precision(1);
        msg << "FED " << fedId_ << " has send " << deltaN << " fragments with CRC errors in the last " << deltaT << " seconds. ";
        msg << "This FED has sent " << fedErrors_.crcErrors << " fragments with CRC errors since the start of the run";
        XCEPT_DECLARE(exception::DataCorruption,sentinelException,msg.str());
        readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
      }
    }
    previousFedErrors_ = fedErrors_;
  }
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

  {
    boost::mutex::scoped_lock sl(fedErrorsMutex_);

    row.add(td(boost::lexical_cast<std::string>(fedErrors_.crcErrors)))
      .add(td(boost::lexical_cast<std::string>(fedErrors_.corruptedEvents)));
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
