#ifndef _evb_readoutunit_Input_h_
#define _evb_readoutunit_Input_h_

#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>

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
#include "evb/DumpUtility.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/Exception.h"
#include "evb/FragmentChain.h"
#include "evb/FragmentTracker.h"
#include "evb/InfoSpaceItems.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/InputMonitor.h"
#include "evb/readoutunit/ScalerHandler.h"
#include "evb/readoutunit/StateMachine.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "log4cplus/loggingmacros.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Reference.h"
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
    class Input
    {
    public:

      Input(ReadoutUnit*);

      virtual ~Input() {};

      /**
       * Notify the proxy of an input source change.
       * The input source is taken from the info space
       * parameter 'inputSource'.
       */
      void inputSourceChanged();

      /**
       * Callback for individual FED fragments received from pt::frl
       */
      void rawDataAvailable(toolbox::mem::Reference*, tcpla::MemoryCache*);

      /**
       * Get the next complete super fragment.
       * If none is available, the method returns false.
       * Otherwise, the FragmentChainPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getNextAvailableSuperFragment(FragmentChainPtr&);

      /**
       * Get the complete super fragment with EvBid.
       * If it is not available or complete, the method returns false.
       * Otherwise, the FragmentChainPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&);

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
       * Return the readout unit associated with this input
       */
      ReadoutUnit* getReadoutUnit() const
      { return readoutUnit_; }


    private:

      void setMasterStream();
      void resetMonitoringCounters();
      void updateSuperFragmentCounters(const FragmentChainPtr&);
      void maybeAddScalerFragment(FragmentChainPtr&);
      cgicc::table getFedTable() const;

      // these methods are only implemented for EVM
      uint32_t getLumiSectionFromTCDS(const unsigned char*) const;
      uint32_t getLumiSectionFromGTP(const unsigned char*) const;
      uint32_t getLumiSectionFromGTPe(const unsigned char*) const;

      ReadoutUnit* readoutUnit_;

      typedef boost::shared_ptr< FerolStream<ReadoutUnit,Configuration> > FerolStreamPtr;
      typedef std::map<uint16_t,FerolStreamPtr> FerolStreams;
      FerolStreams ferolStreams_;
      mutable boost::mutex ferolStreamsMutex_;
      typename FerolStreams::iterator masterStream_;

      boost::scoped_ptr<ScalerHandler> scalerHandler_;

      typedef std::map<uint32_t,uint32_t> LumiCounterMap;
      LumiCounterMap lumiCounterMap_;
      LumiCounterMap::iterator currentLumiCounter_;

      uint32_t runNumber_;

      InputMonitor superFragmentMonitor_;
      mutable boost::mutex superFragmentMonitorMutex_;

      double lastMonitoringTime_;

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
scalerHandler_(new ScalerHandler(readoutUnit->getIdentifier())),
runNumber_(0),
lastMonitoringTime_(0)
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::inputSourceChanged()
{
  configure();
  resetMonitoringCounters();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::rawDataAvailable
(
  toolbox::mem::Reference* bufRef,
  tcpla::MemoryCache* cache
)
{
  FedFragmentPtr fedFragment( new FedFragment(bufRef,cache) );

  const typename FerolStreams::iterator pos = ferolStreams_.find(fedFragment->getFedId());
  if ( pos == ferolStreams_.end() )
  {
    std::ostringstream msg;
    msg << "The received FED id " << fedFragment->getFedId();
    msg << " is not in the excepted FED list: ";
    std::copy(readoutUnit_->getConfiguration()->fedSourceIds.begin(), readoutUnit_->getConfiguration()->fedSourceIds.end(),
              std::ostream_iterator<uint16_t>(msg," "));
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  pos->second->addFedFragment(fedFragment);
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::getNextAvailableSuperFragment(FragmentChainPtr& superFragment)
{
  FedFragmentPtr fedFragment;

  if ( masterStream_ == ferolStreams_.end() || !masterStream_->second->getNextFedFragment(fedFragment) ) return false;

  superFragment.reset( new readoutunit::FragmentChain(fedFragment) );

  for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
       it != itEnd; ++it)
  {
    if ( it != masterStream_ )
    {
      while ( !it->second->getNextFedFragment(fedFragment) ) {};

      superFragment->append(fedFragment);
    }
  }

  maybeAddScalerFragment(superFragment);
  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::getSuperFragmentWithEvBid(const EvBid& evbId, FragmentChainPtr& superFragment)
{
  FedFragmentPtr fedFragment;
  superFragment.reset( new readoutunit::FragmentChain(evbId) );

  for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
       it != itEnd; ++it)
  {
    while ( !it->second->getNextFedFragment(fedFragment) ) {};

    superFragment->append(fedFragment);
  }

  maybeAddScalerFragment(superFragment);
  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::maybeAddScalerFragment
(
  FragmentChainPtr& superFragment
)
{
  FedFragmentPtr fedFragment;
  const EvBid& evbId = superFragment->getEvBid();
  const uint32_t fedSize = scalerHandler_->fillFragment(evbId, fedFragment);
  if ( fedSize > 0 )
  {
    // updateInputMonitors(fedFragment,fedSize);
    // maybeDumpFragmentToFile(fedFragment);
    superFragment->append(fedFragment);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::updateSuperFragmentCounters(const FragmentChainPtr& superFragment)
{
  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    const uint32_t size = superFragment->getSize();
    superFragmentMonitor_.lastEventNumber = superFragment->getEvBid().eventNumber();
    superFragmentMonitor_.perf.sumOfSizes += size;
    superFragmentMonitor_.perf.sumOfSquares += size*size;
    ++superFragmentMonitor_.perf.logicalCount;
    ++superFragmentMonitor_.eventCount;
  }

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
  ++(currentLumiCounter_->second);
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::getEventCountForLumiSection(const uint32_t lumiSection)
{
  const LumiCounterMap::const_iterator pos = lumiCounterMap_.find(lumiSection);

  if ( pos == lumiCounterMap_.end() )
    return 0;
  else
    return pos->second;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  lumiCounterMap_.clear();
  currentLumiCounter_ =
    lumiCounterMap_.insert(LumiCounterMap::value_type(0,0)).first;

  resetMonitoringCounters();

  {
    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      it->second->startProcessing(runNumber);
    }
  }

  scalerHandler_->startProcessing(runNumber);
  runNumber_ = runNumber;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::drain() const
{
  {
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
    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      it->second->stopProcessing();
    }
  }

  scalerHandler_->stopProcessing();
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
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(ferolStreamsMutex_);

    fedIdsWithoutFragments_.clear();
    fedIdsWithErrors_.clear();
    fedDataCorruption_.clear();
    fedCRCerrors_.clear();

    struct timeval time;
    gettimeofday(&time,0);
    const double now = time.tv_sec + static_cast<double>(time.tv_usec) / 1000000;
    const double deltaT = lastMonitoringTime_>0 ? now - lastMonitoringTime_ : 0;
    uint32_t dataReadyCount = 0;
    uint32_t maxElements = 0;

    for (typename FerolStreams::iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
          it != itEnd; ++it)
    {
      uint32_t queueElements = 0;
      uint32_t eventsWithDataCorruption = 0;
      uint32_t eventsWithCRCerrors = 0;

      it->second->retrieveMonitoringQuantities(deltaT,dataReadyCount,queueElements,eventsWithDataCorruption,eventsWithCRCerrors);

      if ( queueElements > maxElements )
        maxElements = queueElements;

      if ( queueElements == 0 )
        fedIdsWithoutFragments_.push_back(it->first);

      if ( eventsWithDataCorruption > 0 || eventsWithCRCerrors > 0 )
      {
        fedIdsWithErrors_.push_back(it->first);
        fedDataCorruption_.push_back(eventsWithDataCorruption);
        fedCRCerrors_.push_back(eventsWithCRCerrors);
      }
    }
    incompleteSuperFragmentCount_ = maxElements;
    dataReadyCount_.value_ += dataReadyCount;
    lastMonitoringTime_ = now;
  }

  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    superFragmentMonitor_.rate = superFragmentMonitor_.perf.logicalRate();
    superFragmentMonitor_.bandwidth = superFragmentMonitor_.perf.bandwidth();
    const uint32_t eventSize = superFragmentMonitor_.perf.size();
    if ( eventSize > 0 )
    {
      superFragmentMonitor_.eventSize = eventSize;
      superFragmentMonitor_.eventSizeStdDev = superFragmentMonitor_.perf.sizeStdDev();
    }
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
    boost::mutex::scoped_lock sl(ferolStreamsMutex_);

    ferolStreams_.clear();
    xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
    for (it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
         it != itEnd; ++it)
    {
      const uint16_t fedId = it->value_;
      if (fedId > FED_COUNT)
      {
        std::ostringstream oss;
        oss << "The fedSourceId " << fedId;
        oss << " is larger than maximal value FED_COUNT=" << FED_COUNT;
        XCEPT_RAISE(exception::Configuration, oss.str());
      }
      ferolStreams_.insert( typename FerolStreams::value_type(fedId,FerolStreamPtr(new FerolStream<ReadoutUnit,Configuration>(readoutUnit_,fedId))) );
    }
  }

  setMasterStream();

  scalerHandler_->configure(configuration->scalFedId,configuration->dummyScalFedSize);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::setMasterStream()
{
  masterStream_ = ferolStreams_.end();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::writeNextFragmentsToFile
(
  const uint16_t count,
  const uint16_t fedId
)
{
  typename FerolStreams::iterator pos;

  if ( fedId == FED_COUNT+1 )
  {
    pos = ferolStreams_.begin();
  }
  else
  {
    pos = ferolStreams_.find(fedId);
  }
  if ( pos != ferolStreams_.end() )
    pos->second->writeNextFragmentsToFile(count);
}


template<class ReadoutUnit,class Configuration>
cgicc::div evb::readoutunit::Input<ReadoutUnit,Configuration>::getHtmlSnipped() const
{
  using namespace cgicc;

  table table;

  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    table.add(tr()
              .add(td("# events"))
              .add(td(boost::lexical_cast<std::string>(superFragmentMonitor_.eventCount))));
    table.add(tr()
              .add(td("evt number of last super fragment"))
              .add(td(boost::lexical_cast<std::string>(superFragmentMonitor_.lastEventNumber))));
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
  }

  table.add(tr()
            .add(td().set("colspan","2")
                 .add(getFedTable())));

  {
    boost::mutex::scoped_lock sl(ferolStreamsMutex_);

    for (typename FerolStreams::const_iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
         it != itEnd; ++it)
    {
      table.add(tr()
                .add(td().set("colspan","2")
                     .add(it->second->getHtmlSnippedForFragmentFIFO())));
    }
  }

  cgicc::div div;
  const std::string javaScript = "function dumpFragments(fedid,count) { var options = { url:'/" +
    readoutUnit_->getURN().toString() + "/writeNextFragmentsToFile?fedid='+fedid+'&count='+count }; xdaqAJAX(options,null); }";
  div.add(script(javaScript).set("type","text/javascript"));
  div.add(p("Input - "+readoutUnit_->getConfiguration()->inputSource.toString()));
  div.add(table);
  return div;
}


template<class ReadoutUnit,class Configuration>
cgicc::table evb::readoutunit::Input<ReadoutUnit,Configuration>::getFedTable() const
{
  using namespace cgicc;

  boost::mutex::scoped_lock sl(ferolStreamsMutex_);

  table fedTable;

  fedTable.add(tr()
               .add(th("Statistics per FED").set("colspan","7")));
  fedTable.add(tr()
               .add(td("FED id").set("colspan","2"))
               .add(td("Last event"))
               .add(td("Size (Bytes)"))
               .add(td("B/w (MB/s)"))
               .add(td("#CRC"))
               .add(td("#corrupt")));

  for (typename FerolStreams::const_iterator it = ferolStreams_.begin(), itEnd = ferolStreams_.end();
       it != itEnd; ++it)
  {
    fedTable.add( it->second->getFedTableRow( it == masterStream_ ) );
  }

  return fedTable;
}


// template<class ReadoutUnit,class Configuration>
// void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::configure(boost::shared_ptr<Configuration> configuration)
// {
//   this->doProcessing_ = false;
//   evbIdFactory_.setFakeLumiSectionDuration(configuration->fakeLumiSectionDuration);

//   toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
//   try
//   {
//     toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
//   }
//   catch(toolbox::mem::exception::MemoryPoolNotFound)
//   {
//     // don't care
//   }

//   try
//   {
//     toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(configuration->fragmentPoolSize.value_);
//     fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
//   }
//   catch(toolbox::mem::exception::Exception& e)
//   {
//     XCEPT_RETHROW(exception::OutOfMemory,
//                   "Failed to create memory pool for dummy fragments", e);
//   }

//   frameSize_ = configuration->frameSize;
//   if ( frameSize_ < FEROL_BLOCK_SIZE )
//   {
//     std::ostringstream oss;
//     oss << "The frame size " << frameSize_ ;
//     oss << " must at least hold one FEROL block of " << FEROL_BLOCK_SIZE << " Bytes";
//     XCEPT_RAISE(exception::Configuration, oss.str());
//   }
//   if ( frameSize_ % FEROL_BLOCK_SIZE != 0 )
//   {
//     std::ostringstream oss;
//     oss << "The frame size " << frameSize_ ;
//     oss << " must be a multiple of the FEROL block size of " << FEROL_BLOCK_SIZE << " Bytes";
//     XCEPT_RAISE(exception::Configuration, oss.str());
//   }

//   fragmentTrackers_.clear();

//   xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
//   for (it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
//        it != itEnd; ++it)
//   {
//     const uint16_t fedId = it->value_;
//     if (fedId > FED_COUNT)
//     {
//       std::ostringstream oss;
//       oss << "The fedSourceId " << fedId;
//       oss << " is larger than maximal value FED_COUNT=" << FED_COUNT;
//       XCEPT_RAISE(exception::Configuration, oss.str());
//     }

//     FragmentTrackerPtr fragmentTracker(
//       new FragmentTracker(fedId,configuration->dummyFedSize,configuration->useLogNormal,
//                           configuration->dummyFedSizeStdDev,configuration->dummyFedSizeMin,configuration->dummyFedSizeMax,configuration->computeCRC)
//     );
//     fragmentTrackers_.insert( FragmentTrackers::value_type(fedId,fragmentTracker) );
//   }
// }


// template<class ReadoutUnit,class Configuration>
// bool evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::createSuperFragment
// (
//   const EvBid& evbId,
//   FragmentChainPtr& superFragment
// )
// {
//   superFragment.reset( new FragmentChain(evbId) );

//   const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);

//   for ( FragmentTrackers::const_iterator it = fragmentTrackers_.begin(), itEnd = fragmentTrackers_.end();
//         it != itEnd; ++it)
//   {
//     toolbox::mem::Reference* fragmentHead = 0;
//     toolbox::mem::Reference* fragmentTail = 0;

//     const uint32_t fedSize = it->second->startFragment(evbId);
//     const uint16_t ferolBlocks = ceil( static_cast<double>(fedSize) / ferolPayloadSize );
//     const uint16_t frameCount = ceil( static_cast<double>(ferolBlocks*FEROL_BLOCK_SIZE) / frameSize_ );
//     uint32_t packetNumber = 0;
//     uint32_t remainingFedSize = fedSize;

//     for (uint16_t frameNb = 0; frameNb < frameCount; ++frameNb)
//     {
//       toolbox::mem::Reference* bufRef = 0;
//       const uint32_t bufSize = frameSize_+sizeof(I2O_DATA_READY_MESSAGE_FRAME);

//       try
//       {
//         bufRef = toolbox::mem::getMemoryPoolFactory()->
//           getFrame(fragmentPool_,bufSize);
//       }
//       catch(xcept::Exception)
//       {
//         return false;
//       }

//       bufRef->setDataSize(frameSize_);
//       memset(bufRef->getDataLocation(), 0, bufSize);
//       I2O_DATA_READY_MESSAGE_FRAME* dataReadyMsg =
//         (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
//       dataReadyMsg->totalLength = fedSize + ferolBlocks*sizeof(ferolh_t);
//       dataReadyMsg->fedid = it->first;
//       dataReadyMsg->triggerno = evbId.eventNumber();
//       uint32_t partLength = 0;

//       unsigned char* frame = (unsigned char*)dataReadyMsg
//         + sizeof(I2O_DATA_READY_MESSAGE_FRAME);

//       while ( remainingFedSize > 0 && partLength+FEROL_BLOCK_SIZE <= frameSize_ )
//       {
//         assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
//         uint32_t length;

//         ferolh_t* ferolHeader = (ferolh_t*)frame;
//         ferolHeader->set_signature();
//         ferolHeader->set_packet_number(packetNumber);

//         if (packetNumber == 0)
//           ferolHeader->set_first_packet();

//         if ( remainingFedSize > ferolPayloadSize )
//         {
//           length = ferolPayloadSize;
//         }
//         else
//         {
//           length = remainingFedSize;
//           ferolHeader->set_last_packet();
//         }
//         remainingFedSize -= length;
//         frame += sizeof(ferolh_t);

//         const size_t filledBytes = it->second->fillData(frame, length);
//         ferolHeader->set_data_length(filledBytes);
//         ferolHeader->set_fed_id(it->first);
//         ferolHeader->set_event_number(evbId.eventNumber());

//         frame += filledBytes;
//         partLength += filledBytes + sizeof(ferolh_t);

//         ++packetNumber;
//         assert(packetNumber < 2048);
//       }

//       dataReadyMsg->partLength = partLength;

//       if ( ! fragmentHead )
//         fragmentHead = bufRef;
//       else
//         fragmentTail->setNextReference(bufRef);
//       fragmentTail = bufRef;

//     }
//     assert( remainingFedSize == 0 );

//     FedFragmentPtr fedFragment( new FedFragment(fragmentHead) );
//     //updateInputMonitors(fedFragment,fedSize);
//     superFragment->append(evbId,fedFragment);
//   }

//   return superFragment->isValid();
// }


// template<class ReadoutUnit,class Configuration>
// void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::startProcessing(const uint32_t runNumber)
// {
//   evbIdFactory_.reset(runNumber);
//   this->doProcessing_ = true;

// }

// template<class ReadoutUnit,class Configuration>
// void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::drain()
// {
//   this->doProcessing_ = false;
// }


// template<class ReadoutUnit,class Configuration>
// void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::stopProcessing()
// {
//   this->doProcessing_ = false;
// }


namespace evb
{
  template<>
  inline uint32_t FragmentChain<I2O_DATA_READY_MESSAGE_FRAME>::calculateSize(toolbox::mem::Reference* bufRef) const
  {
    const unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
    const uint32_t payloadSize = ((I2O_DATA_READY_MESSAGE_FRAME*)payload)->partLength;
    payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

    uint32_t fedSize = 0;
    uint32_t ferolOffset = 0;
    ferolh_t* ferolHeader;

    do
    {
      ferolHeader = (ferolh_t*)(payload + ferolOffset);
      assert( ferolHeader->signature() == FEROL_SIGNATURE );

      const uint32_t dataLength = ferolHeader->data_length();
      fedSize += dataLength;
      ferolOffset += dataLength + sizeof(ferolh_t);
    }
    while ( !ferolHeader->is_last_packet() && ferolOffset < payloadSize );

    return fedSize;
  }


  namespace detail
  {
    template <>
    inline void formatter
    (
      const readoutunit::FragmentChainPtr fragmentChain,
      std::ostringstream* out
    )
    {
      if ( fragmentChain.get() )
      {
        toolbox::mem::Reference* bufRef = fragmentChain->head();
        if ( bufRef )
        {
          I2O_DATA_READY_MESSAGE_FRAME* msg =
            (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
          *out << "I2O_DATA_READY_MESSAGE_FRAME:" << std::endl;
          *out << "  FED id: " << msg->fedid << std::endl;
          *out << "  trigger no: " << msg->triggerno << std::endl;
          *out << "  length: " << msg->partLength <<  "/" << msg->totalLength << std::endl;
          *out << "  evbId: " << fragmentChain->getEvBid() << std::endl;
        }
      }
      else
        *out << "n/a";
    }


    template <>
    inline void formatter
    (
      const FedFragmentPtr fragment,
      std::ostringstream* out
    )
    {
      if ( fragment.get() )
      {
        toolbox::mem::Reference* bufRef = fragment->getBufRef();
        if ( bufRef )
        {
          I2O_DATA_READY_MESSAGE_FRAME* msg =
            (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
          *out << "I2O_DATA_READY_MESSAGE_FRAME:" << std::endl;
          *out << "  FED id: " << msg->fedid << std::endl;
          *out << "  trigger no: " << msg->triggerno << std::endl;
          *out << "  length: " << msg->partLength <<  "/" << msg->totalLength << std::endl;
        }
      }
      else
        *out << "n/a";
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
