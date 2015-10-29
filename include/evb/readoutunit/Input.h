#ifndef _evb_readoutunit_Input_h_
#define _evb_readoutunit_Input_h_

#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "cgicc/HTMLClasses.h"
#include "evb/Constants.h"
#include "evb/DataLocations.h"
#include "evb/DumpUtility.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/Exception.h"
#include "evb/FragmentTracker.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/LocalStream.h"
#include "evb/readoutunit/ScalerStream.h"
#include "evb/readoutunit/InputMonitor.h"
#include "evb/readoutunit/StateMachine.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "log4cplus/loggingmacros.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/WorkLoop.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationContext.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"


namespace evb {

  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Generic input for the readout units
     */
    template<class ReadoutUnit, class Configuration>
    class Input : public toolbox::lang::Class
    {
    public:

      Input(ReadoutUnit*);

      ~Input();

      /**
       * Get the next complete super fragment.
       * If none is available, the method returns false.
       * Otherwise, the SuperFragmentPtr holds a vector of FED fragements.
       */
      bool getNextAvailableSuperFragment(SuperFragmentPtr&);

      /**
       * Get the complete super fragment with EvBid.
       * If it is not available or complete, the method returns false.
       * Otherwise, the SuperFragmentPtr holds a vector of FED fragements.
       */
      void getSuperFragmentWithEvBid(const EvBid&, SuperFragmentPtr&);

      /**
       * Get the number of events contained in the given lumi section
       */
      uint32_t getEventCountForLumiSection(const uint32_t lumiSection);

      /**
       * Append the info space items to be published in the
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);

      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();

      /**
       * Configure
       */
      void configure();

      /**
       * Start processing events
       */
      void startProcessing(const uint32_t runNumber);

      /**
       * Drain the remainig events
       */
      void drain() const;

      /**
       * Stop processing events
       */
      void stopProcessing();

      /**
       * Return monitoring information as cgicc snipped
       */
      cgicc::div getHtmlSnipped() const;

      /**
       * Write the next count FED fragment to a text file
       */
      void writeNextFragmentsToFile(const uint16_t count, const uint16_t fedId=FED_COUNT+1);

      /**
       * Stop generating local events at the given event number
       */
      void stopLocalInputAtEvent(const uint32_t eventNumberToStop);

      /**
       * Adjust the trigger rate when generating local data
       */
      void setMaxTriggerRate(const uint32_t maxTriggerRate);

      /**
       * Return the event rate
       */
      uint32_t getEventRate() const;

      /**
       * Return the number of events received since the start of the run
       */
      uint64_t getEventCount() const;

      /**
       * Return the last event number received
       */
      uint32_t getLastEventNumber() const;

      /**
       * Return the average size of the super fragment
       */
      uint32_t getSuperFragmentSize() const;

      /**
       * Return the readout unit associated with this input
       */
      ReadoutUnit* getReadoutUnit() const
      { return readoutUnit_; }

      typedef boost::shared_ptr< FerolStream<ReadoutUnit,Configuration> > FerolStreamPtr;
      typedef std::map<uint16_t,FerolStreamPtr> FerolStreams;

    private:

      void setMasterStream();
      void resetMonitoringCounters();
      void updateSuperFragmentCounters(const SuperFragmentPtr&);
      void startDummySuperFragmentWorkLoop();
      bool buildDummySuperFragments(toolbox::task::WorkLoop*);
      cgicc::table getFedTable() const;

      // these methods are only implemented for EVM
      uint32_t getLumiSectionFromTCDS(const DataLocations&) const;
      uint32_t getLumiSectionFromGTPe(const DataLocations&) const;

      ReadoutUnit* readoutUnit_;

      FerolStreams ferolStreams_;
      mutable boost::shared_mutex ferolStreamsMutex_;
      typename FerolStreams::iterator masterStream_;

      typedef std::map<uint32_t,uint32_t> LumiCounterMap;
      LumiCounterMap lumiCounterMap_;
      LumiCounterMap::iterator currentLumiCounter_;
      boost::mutex lumiCounterMutex_;

      uint32_t runNumber_;

      toolbox::task::WorkLoop* dummySuperFragmentWL_;
      toolbox::task::ActionSignature* dummySuperFragmentAction_;
      volatile bool buildDummySuperFragmentActive_;

      InputMonitor superFragmentMonitor_;
      mutable boost::mutex superFragmentMonitorMutex_;
      uint32_t incompleteEvents_;

      xdata::UnsignedInteger32 lastEventNumber_;
      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger32 superFragmentSize_;
      xdata::UnsignedInteger32 superFragmentSizeStdDev_;
      xdata::UnsignedInteger32 incompleteSuperFragmentCount_;
      xdata::UnsignedInteger64 eventCount_;
      xdata::UnsignedInteger64 dataReadyCount_;
      xdata::Vector<xdata::UnsignedInteger32> fedIdsWithoutFragments_;
      xdata::Vector<xdata::UnsignedInteger32> fedIdsWithErrors_;
      xdata::Vector<xdata::UnsignedInteger32> fedDataCorruption_;
      xdata::Vector<xdata::UnsignedInteger32> fedCRCerrors_;
      xdata::Vector<xdata::UnsignedInteger32> fedBXerrors_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::Input<ReadoutUnit,Configuration>::Input
(
  ReadoutUnit* readoutUnit
) :
readoutUnit_(readoutUnit),
runNumber_(0),
buildDummySuperFragmentActive_(false),
incompleteEvents_(0)
{}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::Input<ReadoutUnit,Configuration>::~Input()
{
  if ( dummySuperFragmentWL_ && dummySuperFragmentWL_->isActive() )
    dummySuperFragmentWL_->cancel();
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::getNextAvailableSuperFragment(SuperFragmentPtr& superFragment)
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);
    FedFragmentPtr fedFragment;

    if ( masterStream_ == ferolStreams_.end() || !masterStream_->second->getNextFedFragment(fedFragment) ) return false;

    superFragment.reset( new SuperFragment(fedFragment->getEvBid()) );
    superFragment->append(fedFragment);

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      if ( it != masterStream_ )
      {
        it->second->appendFedFragment(superFragment);
      }
    }
  }

  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::getSuperFragmentWithEvBid(const EvBid& evbId, SuperFragmentPtr& superFragment)
{
  superFragment.reset( new SuperFragment(evbId) );

  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);
    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      it->second->appendFedFragment(superFragment);
    }
  }

  updateSuperFragmentCounters(superFragment);
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::buildDummySuperFragments(toolbox::task::WorkLoop* wl)
{
  if ( ferolStreams_.empty() ) return false;

  FedFragmentPtr fedFragment;

  try
  {
    while (1)
    {
      typename FerolStreams::iterator it = ferolStreams_.begin();
      if ( it->second->getNextFedFragment(fedFragment) )
      {
        buildDummySuperFragmentActive_ = true;
        uint32_t size = fedFragment->getFedSize();
        const uint32_t eventNumber = fedFragment->getEventNumber();

        while ( ++it != ferolStreams_.end() )
        {
          while ( ! it->second->getNextFedFragment(fedFragment) ) { sched_yield(); }
          size += fedFragment->getFedSize();
        }

        {
          boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
          superFragmentMonitor_.lastEventNumber = eventNumber;
          superFragmentMonitor_.perf.sumOfSizes += size;
          superFragmentMonitor_.perf.sumOfSquares += size*size;
          ++superFragmentMonitor_.perf.logicalCount;
          ++superFragmentMonitor_.eventCount;
        }
      }
      else
      {
        buildDummySuperFragmentActive_ = false;
        ::usleep(10);
      }
    }
  }
  catch(exception::HaltRequested)
  {
    buildDummySuperFragmentActive_ = false;
    return false;
  }

  buildDummySuperFragmentActive_ = false;
  return true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::updateSuperFragmentCounters(const SuperFragmentPtr& superFragment)
{
  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    const uint32_t size = superFragment->getSize();
    superFragmentMonitor_.lastEventNumber = superFragment->getEvBid().eventNumber();
    superFragmentMonitor_.perf.sumOfSizes += size;
    superFragmentMonitor_.perf.sumOfSquares += size*size;
    ++superFragmentMonitor_.perf.logicalCount;
    ++superFragmentMonitor_.eventCount;
    if ( superFragment->hasMissingFEDs() )
      ++incompleteEvents_;
  }

  {
    boost::mutex::scoped_lock sl(lumiCounterMutex_);

    const uint32_t lumiSection = superFragment->getEvBid().lumiSection();
    for(uint32_t ls = currentLumiCounter_->first+1; ls <= lumiSection; ++ls)
    {
      const std::pair<LumiCounterMap::iterator,bool> result =
        lumiCounterMap_.insert(LumiCounterMap::value_type(ls,0));
      if ( ! result.second )
      {
        std::ostringstream msg;
        msg << "Received an event from lumi section " << ls;
        msg << " for which an entry in lumiCounterMap already exists.";
        XCEPT_RAISE(exception::EventOrder,msg.str());
      }
      currentLumiCounter_ = result.first;
    }
  }
  ++(currentLumiCounter_->second);
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getEventCountForLumiSection(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(lumiCounterMutex_);

  const LumiCounterMap::const_iterator pos = lumiCounterMap_.find(lumiSection);

  if ( pos == lumiCounterMap_.end() )
    return 0;
  else
    return pos->second;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;

  {
    boost::mutex::scoped_lock sl(lumiCounterMutex_);

    lumiCounterMap_.clear();
    currentLumiCounter_ =
      lumiCounterMap_.insert(LumiCounterMap::value_type(0,0)).first;
  }

  resetMonitoringCounters();

  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      it->second->startProcessing(runNumber);
    }
  }

  if ( readoutUnit_->getConfiguration()->dropInputData )
  {
    dummySuperFragmentWL_->submit(dummySuperFragmentAction_);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::drain() const
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    for (typename FerolStreams::const_iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      it->second->drain();
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::stopProcessing()
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      it->second->stopProcessing();
    }
  }
  while ( buildDummySuperFragmentActive_ ) ::usleep(1000);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::stopLocalInputAtEvent(const uint32_t eventNumberToStop)
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      it->second->setEventNumberToStop(eventNumberToStop);
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::setMaxTriggerRate(const uint32_t rate)
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      it->second->setMaxTriggerRate(rate);
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::appendMonitoringItems(InfoSpaceItems& items)
{
  lastEventNumber_ = 0;
  eventRate_ = 0;
  superFragmentSize_ = 0;
  superFragmentSizeStdDev_ = 0;
  incompleteSuperFragmentCount_ = 0;
  eventCount_ = 0;
  dataReadyCount_ = 0;
  fedIdsWithoutFragments_.clear();
  fedIdsWithErrors_.clear();
  fedDataCorruption_.clear();
  fedCRCerrors_.clear();
  fedBXerrors_.clear();

  items.add("lastEventNumber", &lastEventNumber_);
  items.add("eventRate", &eventRate_);
  items.add("superFragmentSize", &superFragmentSize_);
  items.add("superFragmentSizeStdDev", &superFragmentSizeStdDev_);
  items.add("incompleteSuperFragmentCount", &incompleteSuperFragmentCount_);
  items.add("eventCount", &eventCount_);
  items.add("dataReadyCount", &dataReadyCount_);
  items.add("fedIdsWithoutFragments", &fedIdsWithoutFragments_);
  items.add("fedIdsWithErrors", &fedIdsWithErrors_);
  items.add("fedDataCorruption", &fedDataCorruption_);
  items.add("fedCRCerrors", &fedCRCerrors_);
  items.add("fedBXerrors", &fedBXerrors_);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::updateMonitoringItems()
{
  {
    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    fedIdsWithoutFragments_.clear();
    fedIdsWithErrors_.clear();
    fedDataCorruption_.clear();
    fedCRCerrors_.clear();
    fedBXerrors_.clear();

    uint32_t dataReadyCount = 0;
    uint32_t maxElements = 0;

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      uint32_t queueElements = 0;
      uint32_t corruptedEvents = 0;
      uint32_t crcErrors = 0;
      uint32_t bxErrors = 0;

      it->second->retrieveMonitoringQuantities(dataReadyCount,queueElements,corruptedEvents,crcErrors,bxErrors);

      if ( queueElements > maxElements )
        maxElements = queueElements;

      if ( queueElements == 0 )
        fedIdsWithoutFragments_.push_back(it->first);

      if ( corruptedEvents > 0 || crcErrors > 0 || bxErrors > 0 )
      {
        fedIdsWithErrors_.push_back(it->first);
        fedDataCorruption_.push_back(corruptedEvents);
        fedCRCerrors_.push_back(crcErrors);
        fedBXerrors_.push_back(bxErrors);
      }
    }
    incompleteSuperFragmentCount_ = maxElements;
    dataReadyCount_.value_ = dataReadyCount;
  }

  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    const double deltaT = superFragmentMonitor_.perf.deltaT();
    superFragmentMonitor_.rate = superFragmentMonitor_.perf.logicalRate(deltaT);
    superFragmentMonitor_.bandwidth = superFragmentMonitor_.perf.bandwidth(deltaT);
    const uint32_t eventSize = superFragmentMonitor_.perf.size();
    superFragmentMonitor_.eventSize = eventSize;
    superFragmentMonitor_.eventSizeStdDev = superFragmentMonitor_.perf.sizeStdDev();
    superFragmentMonitor_.perf.reset();

    lastEventNumber_ = superFragmentMonitor_.lastEventNumber;
    eventRate_ = superFragmentMonitor_.rate;
    superFragmentSize_ = superFragmentMonitor_.eventSize;
    superFragmentSizeStdDev_ = superFragmentMonitor_.eventSizeStdDev;
    eventCount_ = superFragmentMonitor_.eventCount;
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
  superFragmentMonitor_.reset();

  incompleteEvents_ = 0;
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getEventRate() const
{
  boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
  return superFragmentMonitor_.rate;
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getLastEventNumber() const
{
  boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
  return superFragmentMonitor_.lastEventNumber;
}


template<class ReadoutUnit,class Configuration>
uint64_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getEventCount() const
{
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
    return superFragmentMonitor_.eventCount;
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getSuperFragmentSize() const
{
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);
    return superFragmentMonitor_.eventSize;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::configure()
{
  const boost::shared_ptr<Configuration> configuration = readoutUnit_->getConfiguration();

  if ( configuration->blockSize % 8 != 0 )
  {
    XCEPT_RAISE(exception::Configuration, "The block size must be a multiple of 64-bits");
  }

  {
    boost::unique_lock<boost::shared_mutex> ul(ferolStreamsMutex_);

    ferolStreams_.clear();

    if ( configuration->inputSource == "Socket" )
    {
      readoutUnit_->getFerolConnectionManager()->getActiveFerolStreams(ferolStreams_);
    }
    else
    {
      typename Configuration::FerolSources::const_iterator it, itEnd;
      for (it = configuration->ferolSources.begin(),
             itEnd = configuration->ferolSources.end(); it != itEnd; ++it)
      {
        const uint16_t fedId = it->bag.fedId.value_;
        if (fedId > FED_COUNT)
        {
          std::ostringstream msg;
          msg << "The FED " << fedId;
          msg << " is larger than maximal value FED_COUNT=" << FED_COUNT;
          XCEPT_RAISE(exception::Configuration, msg.str());
        }

        FerolStreamPtr ferolStream;

        if ( configuration->inputSource == "FEROL" )
          ferolStream.reset( new FerolStream<ReadoutUnit,Configuration>(readoutUnit_,fedId) );
        else if ( configuration->inputSource == "Local" )
          ferolStream.reset( new LocalStream<ReadoutUnit,Configuration>(readoutUnit_,fedId) );
        else
          XCEPT_RAISE(exception::Configuration, "Unknown inputSource '"+configuration->inputSource.toString()+"'");

        ferolStreams_.insert( typename FerolStreams::value_type(fedId,ferolStream) );
      }
    }

    setMasterStream();

    if ( configuration->dummyScalFedSize.value_ > 0 )
    {
      const FerolStreamPtr scalerStream( new ScalerStream<ReadoutUnit,Configuration>(readoutUnit_,configuration->scalFedId) );
      ferolStreams_.insert( typename FerolStreams::value_type(configuration->scalFedId,scalerStream ) );
    }

    if ( configuration->dropInputData )
    {
      startDummySuperFragmentWorkLoop();
    }
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::setMasterStream()
{
  masterStream_ = ferolStreams_.end();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::startDummySuperFragmentWorkLoop()
{
  try
  {
    dummySuperFragmentWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( readoutUnit_->getIdentifier("dummySuperFragment"), "waiting" );

    dummySuperFragmentAction_ =
      toolbox::task::bind(this, &evb::readoutunit::Input<ReadoutUnit,Configuration>::buildDummySuperFragments,
                          readoutUnit_->getIdentifier("buildDummySuperFragments") );

    if ( ! dummySuperFragmentWL_->isActive() )
      dummySuperFragmentWL_->activate();
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'dummySuperFragment'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::writeNextFragmentsToFile
(
  const uint16_t count,
  const uint16_t fedId
)
{
  boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);
  typename FerolStreams::iterator pos;

  if ( fedId == FED_COUNT+1 )
    pos = ferolStreams_.begin();
  else
    pos = ferolStreams_.find(fedId);

  if ( pos != ferolStreams_.end() )
    pos->second->writeNextFragmentsToFile(count);
}


template<class ReadoutUnit,class Configuration>
cgicc::div evb::readoutunit::Input<ReadoutUnit,Configuration>::getHtmlSnipped() const
{
  using namespace cgicc;

  cgicc::div div;
  const std::string javaScript = "function dumpFragments(fedid,count) { var options = { url:'/" +
    readoutUnit_->getURN().toString() + "/writeNextFragmentsToFile?fedid='+fedid+'&count='+count }; xdaqAJAX(options,null); }";
  div.add(script(javaScript).set("type","text/javascript"));
  div.add(p("Input - "+readoutUnit_->getConfiguration()->inputSource.toString()));


  {
    table table;
    table.set("title","Super-fragment statistics are only filled when super-fragments have been built. When there are no requests from the BUs, these counters remain 0.");

    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    table.add(tr()
              .add(td("evt number of last super fragment"))
              .add(td(boost::lexical_cast<std::string>(superFragmentMonitor_.lastEventNumber))));
    table.add(tr().set("title","Number of successfully built super fragments")
              .add(td("# super fragments"))
              .add(td(boost::lexical_cast<std::string>(superFragmentMonitor_.eventCount))));
    table.add(tr()
              .add(td("# incomplete super fragments"))
              .add(td(boost::lexical_cast<std::string>(incompleteSuperFragmentCount_.value_))));
    table.add(tr()
              .add(td("# events with missing FEDs"))
              .add(td(boost::lexical_cast<std::string>(incompleteEvents_))));
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(2);
      str << superFragmentMonitor_.bandwidth / 1e6;
      table.add(tr()
                .add(td("throughput (MB/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::scientific);
      str.precision(4);
      str << superFragmentMonitor_.rate;
      table.add(tr()
                .add(td("rate (events/s)"))
                .add(td(str.str())));
    }
    {
      std::ostringstream str;
      str.setf(std::ios::fixed);
      str.precision(1);
      str << superFragmentMonitor_.eventSize / 1e3 << " +/- " << superFragmentMonitor_.eventSizeStdDev / 1e3;
      table.add(tr()
                .add(td("super fragment size (kB)"))
                .add(td(str.str())));
    }
    div.add(table);
  }

  {
    cgicc::div ferolStreams;
    ferolStreams.set("title","Any fragments received from the FEROL are accounted here. If any fragment FIFO is empty, the EvB waits for data from the given FED.");

    boost::shared_lock<boost::shared_mutex> sl(ferolStreamsMutex_);

    ferolStreams.add(getFedTable());

    for (typename FerolStreams::const_iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      ferolStreams.add(it->second->getHtmlSnippedForFragmentFIFO());
    }
    div.add(ferolStreams);
  }

  return div;
}


template<class ReadoutUnit,class Configuration>
cgicc::table evb::readoutunit::Input<ReadoutUnit,Configuration>::getFedTable() const
{
  using namespace cgicc;

  table fedTable;

  fedTable.add(colgroup().add(col().set("span","8")));
  fedTable.add(tr()
               .add(th("Statistics per FED").set("colspan","8")));
  fedTable.add(tr()
               .add(td("FED id").set("colspan","2"))
               .add(td("Last event"))
               .add(td("Size (Bytes)"))
               .add(td("B/w (MB/s)"))
               .add(td("#CRC"))
               .add(td("#corrupt"))
               .add(td("#OOS"))
               .add(td("#BX")));

  for (typename FerolStreams::const_iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
       it != itEnd; ++it)
  {
    fedTable.add( it->second->getFedTableRow() );
  }

  return fedTable;
}


namespace evb
{
  namespace detail
  {
    template <>
    inline void formatter
    (
      const readoutunit::FedFragmentPtr fragment,
      std::ostringstream* out
    )
    {
      if ( fragment.get() )
      {
        *out << "FED fragment:" << std::endl;
        *out << "  FED id: " << fragment->getFedId() << std::endl;
        *out << "  trigger no: " << fragment->getEventNumber() << std::endl;
        *out << "  FED size: " << fragment->getFedSize() << std::endl;
      }
      else
      {
        *out << "n/a";
      }
    }
  } // namespace detail
}


#endif // _evb_readoutunit_Input_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
