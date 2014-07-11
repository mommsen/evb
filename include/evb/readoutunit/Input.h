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

    protected:

      class Handler
      {
      public:

        Handler() : doProcessing_(false) {};

        virtual void addFedFragment(FedFragmentPtr&)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::addFedFragment is not implemented"); }

        virtual bool getNextAvailableSuperFragment(FragmentChainPtr&)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::getNextAvailableSuperFragment is not implemented"); }

        virtual bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::getSuperFragmentWithEvBid is not implemented"); }

        virtual void configure(boost::shared_ptr<Configuration>) {};
        virtual void startProcessing(const uint32_t runNumber) {};
        virtual void drain() {};
        virtual void stopProcessing() {};
        virtual bool isEmpty() const { return true; }
        virtual cgicc::div getHtmlSnippedForFragmentFIFOs() const { return cgicc::div(" "); }
        virtual uint32_t getInfoFromFragmentFIFOs(xdata::Vector<xdata::UnsignedInteger32>& fedIds) const { return 0; }

      protected:

        volatile bool doProcessing_;

      };

      class FEROLproxy : public Handler
      {
      public:

        FEROLproxy(ReadoutUnit*);

        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void addFedFragment(FedFragmentPtr&);
        virtual void startProcessing(const uint32_t runNumber);
        virtual void drain();
        virtual void stopProcessing();
        virtual bool isEmpty() const;
        virtual cgicc::div getHtmlSnippedForFragmentFIFOs() const;
        virtual uint32_t getInfoFromFragmentFIFOs(xdata::Vector<xdata::UnsignedInteger32>& fedIds) const;

      protected:


        ReadoutUnit* readoutUnit_;

        typedef OneToOneQueue<FedFragmentPtr> FragmentFIFO;
        typedef boost::shared_ptr<FragmentFIFO> FragmentFIFOPtr;
        typedef std::map<uint16_t,FragmentFIFOPtr> FragmentFIFOs;
        FragmentFIFOs fragmentFIFOs_;

        typedef std::map<uint16_t,EvBidFactory> EvBidFactories;
        EvBidFactories evbIdFactories_;
        boost::function< uint32_t(const unsigned char*) > lumiSectionFunction_;

      private:

        bool dropInputData_;

      };

      class DummyInputData : public Handler
      {
      public:

        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void startProcessing(const uint32_t runNumber);
        virtual void drain();
        virtual void stopProcessing();

      protected:

        bool createSuperFragment(const EvBid&, FragmentChainPtr&);

        EvBidFactory evbIdFactory_;

      private:

        typedef std::map<uint16_t,FragmentTrackerPtr> FragmentTrackers;
        FragmentTrackers fragmentTrackers_;
        toolbox::mem::Pool* fragmentPool_;
        uint32_t frameSize_;
      };

      virtual void getHandlerForInputSource(boost::shared_ptr<Handler>& handler)
      { handler.reset( new Handler() ); }

      const boost::shared_ptr<Configuration> configuration_;

    private:

      void resetMonitoringCounters();
      void maybeDumpFragmentToFile(const FedFragmentPtr&);
      void writeFragmentToFile(const FedFragmentPtr&,const std::string& reasonFordump) const;
      void updateInputMonitors(const FedFragmentPtr&,const uint32_t fedSize);
      void updateSuperFragmentCounters(const FragmentChainPtr&);
      void maybeAddScalerFragment(FragmentChainPtr&);
      cgicc::table getFedTable() const;

      ReadoutUnit* readoutUnit_;
      boost::shared_ptr<Handler> handler_;
      boost::scoped_ptr<ScalerHandler> scalerHandler_;

      typedef std::map<uint32_t,uint32_t> LumiCounterMap;
      LumiCounterMap lumiCounterMap_;
      LumiCounterMap::iterator currentLumiCounter_;

      uint32_t runNumber_;

      typedef std::pair<uint16_t,uint16_t> CountAndFedId;
      CountAndFedId writeNextFragmentsForFedId_;

      struct InputMonitor
      {
        uint32_t lastEventNumber;
        uint64_t eventCount;
        PerformanceMonitor perf;
        double rate;
        double eventSize;
        double eventSizeStdDev;
        double bandwidth;

        InputMonitor() :
          lastEventNumber(0),eventCount(0),rate(0),eventSize(0),eventSizeStdDev(0),bandwidth(0) {};
      };
      typedef std::map<uint16_t,InputMonitor> InputMonitors;
      InputMonitors inputMonitors_;
      mutable boost::mutex inputMonitorsMutex_;

      InputMonitor superFragmentMonitor_;
      mutable boost::mutex superFragmentMonitorMutex_;

      struct FedErrors
      {
        uint32_t corruptedEvents;
        uint32_t crcErrors;

        FedErrors() :
          corruptedEvents(0),crcErrors(0) {};
      };
      typedef std::map<uint16_t,FedErrors> FedErrorCounts;
      FedErrorCounts fedErrorCounts_;
      FedErrorCounts previousFedErrorCounts_;
      mutable boost::mutex fedErrorCountsMutex_;
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
configuration_(readoutUnit->getConfiguration()),
readoutUnit_(readoutUnit),
handler_(new Handler()),
scalerHandler_(new ScalerHandler(readoutUnit->getIdentifier())),
runNumber_(0),
lastMonitoringTime_(0)
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::inputSourceChanged()
{
  getHandlerForInputSource(handler_);
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
  uint32_t fedSize = 0;

  try
  {
    fedSize = FedFragment::checkIntegrity(bufRef,configuration_->checkCRC);
  }
  catch(exception::DataCorruption& e)
  {
    uint32_t count = 0;
    {
      boost::mutex::scoped_lock sl(fedErrorCountsMutex_);
      count = ++(fedErrorCounts_[fedFragment->getFedId()].corruptedEvents);
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
      boost::mutex::scoped_lock sl(fedErrorCountsMutex_);
      count = ++(fedErrorCounts_[fedFragment->getFedId()].crcErrors);
    }

    if ( count <= configuration_->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());

    LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("error",e);
  }

  updateInputMonitors(fedFragment,fedSize);
  maybeDumpFragmentToFile(fedFragment);
  handler_->addFedFragment(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::updateInputMonitors
(
  const FedFragmentPtr& fragment,
  uint32_t fedSize
)
{
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);

  typename InputMonitors::iterator pos = inputMonitors_.find(fragment->getFedId());
  if ( pos == inputMonitors_.end() )
  {
    std::ostringstream msg;
    msg << "The received FED id " << fragment->getFedId();
    msg << " is not in the excepted FED list: ";
    std::copy(configuration_->fedSourceIds.begin(), configuration_->fedSourceIds.end(),
              std::ostream_iterator<uint16_t>(msg," "));
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  ++(pos->second.perf.i2oCount);
  ++(pos->second.perf.logicalCount);
  pos->second.perf.sumOfSizes += fedSize;
  pos->second.perf.sumOfSquares += fedSize*fedSize;
  pos->second.lastEventNumber = fragment->getEventNumber();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::maybeDumpFragmentToFile
(
  const FedFragmentPtr& fragment
)
{
  if ( configuration_->writeFragmentsToFile ||
       (writeNextFragmentsForFedId_.first > 0 &&
        (writeNextFragmentsForFedId_.second == fragment->getFedId() || writeNextFragmentsForFedId_.second == FED_COUNT+1)) )

  {
    writeFragmentToFile(fragment,"Requested by user");
    --writeNextFragmentsForFedId_.first;
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::writeFragmentToFile
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
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::getNextAvailableSuperFragment(FragmentChainPtr& superFragment)
{
  if ( ! handler_->getNextAvailableSuperFragment(superFragment) ) return false;

  maybeAddScalerFragment(superFragment);
  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::getSuperFragmentWithEvBid(const EvBid& evbId, FragmentChainPtr& superFragment)
{
  if ( ! handler_->getSuperFragmentWithEvBid(evbId,superFragment) ) return false;

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
    updateInputMonitors(fedFragment,fedSize);
    maybeDumpFragmentToFile(fedFragment);
    superFragment->append(evbId,fedFragment);
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
  handler_->startProcessing(runNumber);
  scalerHandler_->startProcessing(runNumber);
  runNumber_ = runNumber;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::drain() const
{
  handler_->drain();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::stopProcessing()
{
  handler_->stopProcessing();
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
  uint32_t dataReadyCount = 0;

  {
    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    for (typename InputMonitors::iterator it = inputMonitors_.begin(), itEnd = inputMonitors_.end();
         it != itEnd; ++it)
    {
      dataReadyCount += it->second.perf.logicalCount;

      it->second.rate = it->second.perf.logicalRate();
      it->second.bandwidth = it->second.perf.bandwidth();
      const uint32_t eventSize = it->second.perf.size();
      if ( eventSize > 0 )
      {
        it->second.eventSize = eventSize;
        it->second.eventSizeStdDev = it->second.perf.sizeStdDev();
      }
      it->second.perf.reset();
    }
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
  }

  lastEventNumber_ = superFragmentMonitor_.lastEventNumber;
  eventRate_ = superFragmentMonitor_.rate;
  superFragmentSize_ = superFragmentMonitor_.eventSize;
  superFragmentSizeStdDev_ = superFragmentMonitor_.eventSizeStdDev;
  eventCount_ = superFragmentMonitor_.eventCount;
  dataReadyCount_.value_ += dataReadyCount;

  incompleteSuperFragmentCount_ =
    handler_->getInfoFromFragmentFIFOs(fedIdsWithoutFragments_);

  {
    boost::mutex::scoped_lock sl(fedErrorCountsMutex_);

    fedIdsWithErrors_.clear();
    fedDataCorruption_.clear();
    fedCRCerrors_.clear();

    struct timeval time;
    gettimeofday(&time,0);
    const double now = time.tv_sec + static_cast<double>(time.tv_usec) / 1000000;
    const double deltaT = lastMonitoringTime_>0 ? now - lastMonitoringTime_ : 0;

    for ( typename FedErrorCounts::const_iterator it = fedErrorCounts_.begin(), itEnd = fedErrorCounts_.end();
          it != itEnd; ++it)
    {
      fedIdsWithErrors_.push_back(it->first);
      fedDataCorruption_.push_back(it->second.corruptedEvents);
      fedCRCerrors_.push_back(it->second.crcErrors);

      const typename FedErrorCounts::const_iterator pos = previousFedErrorCounts_.find(it->first);
      if ( deltaT > 0 && pos != previousFedErrorCounts_.end() )
      {
        const uint32_t deltaN = it->second.crcErrors - pos->second.crcErrors;
        const double rate = deltaN / deltaT;
        if ( rate > configuration_->maxCRCErrorRate )
        {
          std::ostringstream msg;
          msg.setf(std::ios::fixed);
          msg.precision(1);
          msg << "FED " << it->first << " has send " << deltaN << " fragments with CRC errors in the last " << deltaT << " seconds. ";
          msg << "This FED has sent " << it->second.crcErrors << " fragments with CRC errors since the start of the run";
          XCEPT_DECLARE(exception::DataCorruption,sentinelException,msg.str());
          readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
        }
      }
    }
    previousFedErrorCounts_ = fedErrorCounts_;
    lastMonitoringTime_ = now;
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    inputMonitors_.clear();
    xdata::Vector<xdata::UnsignedInteger32>::const_iterator it = configuration_->fedSourceIds.begin();
    const xdata::Vector<xdata::UnsignedInteger32>::const_iterator itEnd = configuration_->fedSourceIds.end();
    for ( ; it != itEnd ; ++it)
    {
      inputMonitors_.insert(typename InputMonitors::value_type(it->value_,InputMonitor()));
    }
    if ( configuration_->dummyScalFedSize.value_ > 0 )
    {
      inputMonitors_.insert(typename InputMonitors::value_type(configuration_->scalFedId,InputMonitor()));
    }
  }
  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    superFragmentMonitor_.lastEventNumber = 0;
    superFragmentMonitor_.eventCount = 0;
    superFragmentMonitor_.rate = 0;
    superFragmentMonitor_.bandwidth = 0;
    superFragmentMonitor_.eventSize = 0;
    superFragmentMonitor_.eventSizeStdDev = 0;
    superFragmentMonitor_.perf.reset();
  }
  {
    boost::mutex::scoped_lock sl(fedErrorCountsMutex_);

    fedErrorCounts_.clear();
    previousFedErrorCounts_.clear();
    lastMonitoringTime_ = 0;
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::configure()
{
  if ( configuration_->blockSize % 8 != 0 )
  {
    XCEPT_RAISE(exception::Configuration, "The block size must be a multiple of 64-bits");
  }

  writeNextFragmentsToFile(0);

  handler_->configure(configuration_);
  scalerHandler_->configure(configuration_->scalFedId,configuration_->dummyScalFedSize);

  resetMonitoringCounters();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::writeNextFragmentsToFile
(
  const uint16_t count,
  const uint16_t fedId
)
{
  writeNextFragmentsForFedId_ = CountAndFedId(count,fedId);
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

  table.add(tr()
            .add(td().set("colspan","2")
                 .add(handler_->getHtmlSnippedForFragmentFIFOs())));

  cgicc::div div;
  const std::string javaScript = "function dumpFragments(fedid,count) { var options = { url:'/" +
    readoutUnit_->getURN().toString() + "/writeNextFragmentsToFile?fedid='+fedid+'&count='+count }; xdaqAJAX(options,null); }";
  div.add(script(javaScript).set("type","text/javascript"));
  div.add(p("Input - "+configuration_->inputSource.toString()));
  div.add(table);
  return div;
}


template<class ReadoutUnit,class Configuration>
cgicc::table evb::readoutunit::Input<ReadoutUnit,Configuration>::getFedTable() const
{
  using namespace cgicc;

  boost::mutex::scoped_lock sl(inputMonitorsMutex_);

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

  typename InputMonitors::const_iterator it, itEnd;
  for (it=inputMonitors_.begin(), itEnd = inputMonitors_.end();
       it != itEnd; ++it)
  {
    const std::string fedId = boost::lexical_cast<std::string>(it->first);

    tr row;
    row.add(td(fedId));
    row.add(td()
            .add(button("dump").set("type","button").set("title","write the next FED fragment to /tmp")
                 .set("onclick","dumpFragments("+fedId+",1);")));
    row.add(td(boost::lexical_cast<std::string>(it->second.lastEventNumber)));
    row.add(td(boost::lexical_cast<std::string>(static_cast<uint32_t>(it->second.eventSize))
               +" +/- "+boost::lexical_cast<std::string>(static_cast<uint32_t>(it->second.eventSizeStdDev))));

    std::ostringstream str;
    str.setf(std::ios::fixed);
    str.precision(1);
    str << it->second.bandwidth / 1e6;
    row.add(td(str.str()));

    const typename FedErrorCounts::const_iterator pos = fedErrorCounts_.find(it->first);
    if ( pos == fedErrorCounts_.end() )
      row.add(td("0")).add(td("0"));
    else
      row.add(td(boost::lexical_cast<std::string>(pos->second.crcErrors)))
        .add(td(boost::lexical_cast<std::string>(pos->second.corruptedEvents)));

    fedTable.add(row);
  }

  return fedTable;
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::FEROLproxy(ReadoutUnit* readoutUnit) :
  Handler(),
  readoutUnit_(readoutUnit),
  lumiSectionFunction_(0)
{}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::configure(boost::shared_ptr<Configuration> configuration)
{
  this->doProcessing_ = false;
  dropInputData_ = configuration->dropInputData;

  evbIdFactories_.clear();
  fragmentFIFOs_.clear();

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

    std::ostringstream fifoName;
    fifoName << "fragmentFIFO_FED_" << fedId;
    FragmentFIFOPtr fragmentFIFO( new FragmentFIFO(readoutUnit_,fifoName.str()) );
    fragmentFIFO->resize(configuration->fragmentFIFOCapacity);
    fragmentFIFOs_.insert( typename FragmentFIFOs::value_type(fedId,fragmentFIFO) );
    evbIdFactories_.insert( typename EvBidFactories::value_type(fedId,EvBidFactory()) );
  }
}


template<class ReadoutUnit,class Configuration>
uint32_t evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::getInfoFromFragmentFIFOs
(
  xdata::Vector<xdata::UnsignedInteger32>& fedIds
) const
{
  fedIds.clear();
  uint32_t maxElements = 0;

  FragmentFIFOs::const_iterator it, itEnd;
  for (it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    const uint32_t elements = it->second->elements();
    if ( elements > maxElements )
      maxElements = elements;

    if ( elements == 0 )
      fedIds.push_back(it->first);
  }

  return maxElements;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::addFedFragment
(
  FedFragmentPtr& fedFragment
)
{
  if ( this->doProcessing_ && !dropInputData_ )
    fragmentFIFOs_[fedFragment->getFedId()]->enqWait(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::startProcessing(const uint32_t runNumber)
{
  for (typename EvBidFactories::iterator it = evbIdFactories_.begin(), itEnd = evbIdFactories_.end();
       it != itEnd; ++it)
    it->second.reset(runNumber);

  this->doProcessing_ = true;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::drain()
{
  while ( ! isEmpty() ) ::usleep(1000);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::stopProcessing()
{
  this->doProcessing_ = false;
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::isEmpty() const
{
  for (typename FragmentFIFOs::const_iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    if ( ! it->second->empty() ) return false;
  }

  return true;
}


template<class ReadoutUnit,class Configuration>
cgicc::div evb::readoutunit::Input<ReadoutUnit,Configuration>::FEROLproxy::getHtmlSnippedForFragmentFIFOs() const
{
  using namespace cgicc;

  cgicc::div queues;

  for (typename FragmentFIFOs::const_iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    queues.add(it->second->getHtmlSnipped());
  }
  return queues;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::configure(boost::shared_ptr<Configuration> configuration)
{
  this->doProcessing_ = false;
  evbIdFactory_.setFakeLumiSectionDuration(configuration->fakeLumiSectionDuration);

  toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
  try
  {
    toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
  }
  catch(toolbox::mem::exception::MemoryPoolNotFound)
  {
    // don't care
  }

  try
  {
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(configuration->fragmentPoolSize.value_);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for dummy fragments", e);
  }

  frameSize_ = configuration->frameSize;
  if ( frameSize_ < FEROL_BLOCK_SIZE )
  {
    std::ostringstream oss;
    oss << "The frame size " << frameSize_ ;
    oss << " must at least hold one FEROL block of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  if ( frameSize_ % FEROL_BLOCK_SIZE != 0 )
  {
    std::ostringstream oss;
    oss << "The frame size " << frameSize_ ;
    oss << " must be a multiple of the FEROL block size of " << FEROL_BLOCK_SIZE << " Bytes";
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  fragmentTrackers_.clear();

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

    FragmentTrackerPtr fragmentTracker(
      new FragmentTracker(fedId,configuration->dummyFedSize,configuration->useLogNormal,
                          configuration->dummyFedSizeStdDev,configuration->dummyFedSizeMin,configuration->dummyFedSizeMax,configuration->computeCRC)
    );
    fragmentTrackers_.insert( FragmentTrackers::value_type(fedId,fragmentTracker) );
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::createSuperFragment
(
  const EvBid& evbId,
  FragmentChainPtr& superFragment
)
{
  superFragment.reset( new FragmentChain(evbId) );

  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);

  for ( FragmentTrackers::const_iterator it = fragmentTrackers_.begin(), itEnd = fragmentTrackers_.end();
        it != itEnd; ++it)
  {
    toolbox::mem::Reference* fragmentHead = 0;
    toolbox::mem::Reference* fragmentTail = 0;

    const uint32_t fedSize = it->second->startFragment(evbId);
    const uint16_t ferolBlocks = ceil( static_cast<double>(fedSize) / ferolPayloadSize );
    const uint16_t frameCount = ceil( static_cast<double>(ferolBlocks*FEROL_BLOCK_SIZE) / frameSize_ );
    uint32_t packetNumber = 0;
    uint32_t remainingFedSize = fedSize;

    for (uint16_t frameNb = 0; frameNb < frameCount; ++frameNb)
    {
      toolbox::mem::Reference* bufRef = 0;
      const uint32_t bufSize = frameSize_+sizeof(I2O_DATA_READY_MESSAGE_FRAME);

      try
      {
        bufRef = toolbox::mem::getMemoryPoolFactory()->
          getFrame(fragmentPool_,bufSize);
      }
      catch(xcept::Exception)
      {
        return false;
      }

      bufRef->setDataSize(frameSize_);
      memset(bufRef->getDataLocation(), 0, bufSize);
      I2O_DATA_READY_MESSAGE_FRAME* dataReadyMsg =
        (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
      dataReadyMsg->totalLength = fedSize + ferolBlocks*sizeof(ferolh_t);
      dataReadyMsg->fedid = it->first;
      dataReadyMsg->triggerno = evbId.eventNumber();
      uint32_t partLength = 0;

      unsigned char* frame = (unsigned char*)dataReadyMsg
        + sizeof(I2O_DATA_READY_MESSAGE_FRAME);

      while ( remainingFedSize > 0 && partLength+FEROL_BLOCK_SIZE <= frameSize_ )
      {
        assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
        uint32_t length;

        ferolh_t* ferolHeader = (ferolh_t*)frame;
        ferolHeader->set_signature();
        ferolHeader->set_packet_number(packetNumber);

        if (packetNumber == 0)
          ferolHeader->set_first_packet();

        if ( remainingFedSize > ferolPayloadSize )
        {
          length = ferolPayloadSize;
        }
        else
        {
          length = remainingFedSize;
          ferolHeader->set_last_packet();
        }
        remainingFedSize -= length;
        frame += sizeof(ferolh_t);

        const size_t filledBytes = it->second->fillData(frame, length);
        ferolHeader->set_data_length(filledBytes);
        ferolHeader->set_fed_id(it->first);
        ferolHeader->set_event_number(evbId.eventNumber());

        frame += filledBytes;
        partLength += filledBytes + sizeof(ferolh_t);

        ++packetNumber;
        assert(packetNumber < 2048);
      }

      dataReadyMsg->partLength = partLength;

      if ( ! fragmentHead )
        fragmentHead = bufRef;
      else
        fragmentTail->setNextReference(bufRef);
      fragmentTail = bufRef;

    }
    assert( remainingFedSize == 0 );

    FedFragmentPtr fedFragment( new FedFragment(fragmentHead) );
    updateInputMonitors(fedFragment,fedSize);
    superFragment->append(evbId,fedFragment);
  }

  return superFragment->isValid();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::startProcessing(const uint32_t runNumber)
{
  evbIdFactory_.reset(runNumber);
  this->doProcessing_ = true;

}

template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::drain()
{
  this->doProcessing_ = false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::Input<ReadoutUnit,Configuration>::DummyInputData::stopProcessing()
{
  this->doProcessing_ = false;
}


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
