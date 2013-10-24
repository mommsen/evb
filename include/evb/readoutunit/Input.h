#ifndef _evb_readoutunit_Input_h_
#define _evb_readoutunit_Input_h_

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <iterator>
#include <map>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "evb/CRCCalculator.h"
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
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"
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

    typedef FragmentChain<I2O_DATA_READY_MESSAGE_FRAME> FragmentChain;
    typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;

    /**
     * \ingroup xdaqApps
     * \brief Generic input for the readout units
     */
    template<class Configuration>
    class Input
    {
    public:

      Input
      (
        xdaq::ApplicationStub*,
        boost::shared_ptr<Configuration>
      );

      virtual ~Input() {};

      /**
       * Notify the proxy of an input source change.
       * The input source is taken from the info space
       * parameter 'inputSource'.
       */
      void inputSourceChanged();

      /**
       * Callback for I2O_SUPER_FRAGMENT_READY messages received from pt::frl
       */
      void superFragmentReady(toolbox::mem::Reference*);

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
      void drain();

      /**
       * Stop processing events
       */
      void stopProcessing();

      /**
       * Print monitoring information as HTML snipped
       */
      void printHtml(xgi::Output*) const;

      /**
       * Print the content of the super-fragment FIFO as HTML snipped
       */
      void printSuperFragmentFIFO(xgi::Output* out) const;

      /**
       * Check the consistency of the FED event fragment
       */
      void checkEventFragment(toolbox::mem::Reference*);


    protected:

      class Handler
      {
      public:

        Handler(Input<Configuration>* input)
        : input_(input), doProcessing_(false) {};

        virtual void superFragmentReady(toolbox::mem::Reference*)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::superFragmentReady is not implemented"); }

        virtual void rawDataAvailable(toolbox::mem::Reference*, tcpla::MemoryCache*)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::rawDataAvailable is not implemented"); }

        virtual bool getNextAvailableSuperFragment(FragmentChainPtr&)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::getNextAvailableSuperFragment is not implemented"); }

        virtual bool getSuperFragmentWithEvBid(const EvBid&, FragmentChainPtr&)
        { XCEPT_RAISE(exception::Configuration, "readoutunit::Input::Handler::getSuperFragmentWithEvBid is not implemented"); }

        virtual void configure(boost::shared_ptr<Configuration>) {};
        virtual void startProcessing(const uint32_t runNumber) {};
        virtual void drain() {};
        virtual void stopProcessing() {};
        virtual void printSuperFragmentFIFO(xgi::Output*) const {};
        virtual void printFragmentFIFOs(xgi::Output*, const toolbox::net::URN&) const {};

      protected:

        Input<Configuration>* input_;
        bool doProcessing_;

      };

      class FEROLproxy : public Handler
      {
      public:

        FEROLproxy(Input<Configuration>*);

        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void superFragmentReady(toolbox::mem::Reference*);
        virtual void rawDataAvailable(toolbox::mem::Reference*, tcpla::MemoryCache*);
        virtual void startProcessing(const uint32_t runNumber);
        virtual void drain();
        virtual void stopProcessing();
        virtual void printSuperFragmentFIFO(xgi::Output*) const;
        virtual void printFragmentFIFOs(xgi::Output*, const toolbox::net::URN&) const;

      protected:

        virtual uint32_t extractTriggerInformation(const unsigned char*) const
        { return 0; }

        typedef OneToOneQueue<FragmentChainPtr> SuperFragmentFIFO;
        SuperFragmentFIFO superFragmentFIFO_;

        typedef OneToOneQueue<FragmentChain::FragmentPtr> FragmentFIFO;
        typedef boost::shared_ptr<FragmentFIFO> FragmentFIFOPtr;
        typedef std::map<uint16_t,FragmentFIFOPtr> FragmentFIFOs;
        FragmentFIFOs fragmentFIFOs_;

      private:

        bool dropInputData_;

        typedef std::map<uint16_t,EvBidFactory> EvBidFactories;
        EvBidFactories evbIdFactories_;

      };

      class DummyInputData : public Handler
      {
      public:

        DummyInputData(Input<Configuration>* input) : Handler(input) {};

        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void startProcessing(const uint32_t runNumber);
        virtual void stopProcessing();

      protected:

        bool createSuperFragment(const EvBid&, FragmentChainPtr&);

        EvBidFactory evbIdFactory_;
        uint32_t eventNumber_;

      private:

        typedef std::map<uint16_t,FragmentTrackerPtr> FragmentTrackers;
        FragmentTrackers fragmentTrackers_;
        toolbox::mem::Pool* fragmentPool_;
        uint32_t frameSize_;
      };

      virtual void getHandlerForInputSource(boost::shared_ptr<Handler>& handler)
      { handler.reset( new Handler(this) ); }

      const boost::shared_ptr<Configuration> configuration_;

    private:

      void resetMonitoringCounters();
      void dumpFragmentToLogger(toolbox::mem::Reference*) const;
      void updateSuperFragmentCounters(const FragmentChainPtr&);

      xdaq::ApplicationStub* app_;
      boost::shared_ptr<Handler> handler_;
      CRCCalculator crcCalculator_;

      struct InputMonitoring
      {
        uint32_t lastEventNumber;
        PerformanceMonitor perf;
        double rate;
        double eventSize;
        double eventSizeStdDev;
        double bandwidth;
      };
      typedef std::map<uint16_t,InputMonitoring> InputMonitors;
      InputMonitors inputMonitors_;
      mutable boost::mutex inputMonitorsMutex_;

      InputMonitoring superFragmentMonitor_;
      mutable boost::mutex superFragmentMonitorMutex_;

      xdata::UnsignedInteger32 eventRate_;
      xdata::UnsignedInteger32 superFragmentSize_;
      xdata::UnsignedInteger32 superFragmentSizeStdDev_;
      xdata::UnsignedInteger64 dataReadyCount_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class Configuration>
evb::readoutunit::Input<Configuration>::Input
(
  xdaq::ApplicationStub* app,
  boost::shared_ptr<Configuration> configuration
) :
configuration_(configuration),
app_(app),
handler_(new Handler(this))
{}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::inputSourceChanged()
{
  getHandlerForInputSource(handler_);
  handler_->configure(configuration_);
  resetMonitoringCounters();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::superFragmentReady(toolbox::mem::Reference* bufRef)
{
  checkEventFragment(bufRef);

  if ( configuration_->dumpFragmentsToLogger )
    dumpFragmentToLogger(bufRef);

  handler_->superFragmentReady(bufRef);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::rawDataAvailable
(
  toolbox::mem::Reference* bufRef,
  tcpla::MemoryCache* cache
)
{
  checkEventFragment(bufRef);

  if ( configuration_->dumpFragmentsToLogger )
    dumpFragmentToLogger(bufRef);

  handler_->rawDataAvailable(bufRef,cache);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::checkEventFragment
(
  toolbox::mem::Reference* bufRef
)
{
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  I2O_DATA_READY_MESSAGE_FRAME* frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
  uint32_t payloadSize = frame->partLength;
  const uint16_t fedId = frame->fedid;
  const uint32_t eventNumber = frame->triggerno;
  uint16_t crc = 0xffff;

  try
  {
    payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

    uint32_t fedSize = 0;
    uint32_t usedSize = 0;
    ferolh_t* ferolHeader;

    do
    {
      ferolHeader = (ferolh_t*)payload;

      if ( ferolHeader->signature() != FEROL_SIGNATURE )
      {
        std::ostringstream msg;
        msg << "Expected FEROL header signature " << std::hex << FEROL_SIGNATURE;
        msg << ", but found " << std::hex << ferolHeader->signature();
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      if ( eventNumber != ferolHeader->event_number() )
      {
        std::ostringstream msg;
        msg << "Mismatch of event number in FEROL header:";
        msg << " expected " << eventNumber << ", but got " << ferolHeader->event_number();
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      if ( fedId != ferolHeader->fed_id() )
      {
        std::ostringstream msg;
        msg << "Mismatch of FED id in FEROL header:";
        msg << " expected " << fedId << ", but got " << ferolHeader->fed_id();
        msg << " for event " << eventNumber;
        XCEPT_RAISE(exception::DataCorruption, msg.str());
      }

      payload += sizeof(ferolh_t);

      if ( ferolHeader->is_first_packet() )
      {
        const fedh_t* fedHeader = (fedh_t*)payload;

        if ( FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER )
        {
          std::ostringstream oss;
          oss << "Expected FED header maker 0x" << std::hex << FED_SLINK_START_MARKER;
          oss << " but got event id 0x" << std::hex << fedHeader->eventid;
          oss << " and source id 0x" << std::hex << fedHeader->sourceid;
          XCEPT_RAISE(exception::DataCorruption, oss.str());
        }

        const uint32_t eventId = FED_LVL1_EXTRACT(fedHeader->eventid);
        if ( eventId != eventNumber)
        {
          std::ostringstream oss;
          oss << "FED header \"eventid\" " << eventId << " does not match";
          oss << " the eventNumber " << eventNumber << " found in I2O_DATA_READY_MESSAGE_FRAME";
          XCEPT_RAISE(exception::DataCorruption, oss.str());
        }

        const uint32_t sourceId = FED_SOID_EXTRACT(fedHeader->sourceid);
        if ( sourceId != fedId )
        {
          std::ostringstream oss;
          oss << "FED header \"sourceId\" " << sourceId << " does not match";
          oss << " the FED id " << fedId << " found in I2O_DATA_READY_MESSAGE_FRAME";
          XCEPT_RAISE(exception::DataCorruption, oss.str());
        }
      }

      const uint32_t dataLength = ferolHeader->data_length();

      if ( configuration_->checkCRC )
      {
        if ( ferolHeader->is_last_packet() )
          crcCalculator_.compute(crc,payload,dataLength-sizeof(fedt_t)); // omit the FED trailer
        else
          crcCalculator_.compute(crc,payload,dataLength);
      }

      payload += dataLength;
      fedSize += dataLength;
      usedSize += dataLength + sizeof(ferolh_t);

      if ( usedSize >= payloadSize && !ferolHeader->is_last_packet() )
      {
        usedSize -= payloadSize;
        bufRef = bufRef->getNextReference();

        if ( ! bufRef )
        {
          std::ostringstream msg;
          msg << "Premature end of FEROL data:";
          msg << " expected " << usedSize << " more Bytes,";
          msg << " but toolbox::mem::Reference chain has ended";
          XCEPT_RAISE(exception::DataCorruption, msg.str());
        }

        payload = (unsigned char*)bufRef->getDataLocation();
        frame = (I2O_DATA_READY_MESSAGE_FRAME*)payload;
        payloadSize = frame->partLength;
        payload += sizeof(I2O_DATA_READY_MESSAGE_FRAME);

        if ( fedId != frame->fedid )
        {
          std::ostringstream msg;
          msg << "Inconsistent FED id:";
          msg << " first I2O_DATA_READY_MESSAGE_FRAME was from FED id " << fedId;
          msg << " while the current has FED id " << frame->fedid;
          XCEPT_RAISE(exception::DataCorruption, msg.str());
        }

        if ( eventNumber != frame->triggerno )
        {
          std::ostringstream msg;
          msg << "Inconsistent event number:";
          msg << " first I2O_DATA_READY_MESSAGE_FRAME was from event " << eventNumber;
          msg << " while the current is from event " << frame->triggerno;
          XCEPT_RAISE(exception::DataCorruption, msg.str());
        }
      }
    }
    while ( !ferolHeader->is_last_packet() );

    fedt_t* trailer = (fedt_t*)(payload - sizeof(fedt_t));

    if ( FED_TCTRLID_EXTRACT(trailer->eventsize) != FED_SLINK_END_MARKER )
    {
      std::ostringstream oss;
      oss << "Expected FED trailer 0x" << std::hex << FED_SLINK_END_MARKER;
      oss << " but got event size 0x" << std::hex << trailer->eventsize;
      oss << " and conscheck 0x" << std::hex << trailer->conscheck;
      XCEPT_RAISE(exception::DataCorruption, oss.str());
    }

    const uint32_t evsz = FED_EVSZ_EXTRACT(trailer->eventsize)<<3;
    if ( evsz != fedSize )
    {
      std::ostringstream oss;
      oss << "Inconsistent event size:";
      oss << " FED trailer claims " << evsz << " Bytes,";
      oss << " while sum of FEROL headers yield " << fedSize;
      XCEPT_RAISE(exception::DataCorruption, oss.str());
    }

    if ( configuration_->checkCRC )
    {
      // Force CRC & R field to zero before re-computing the CRC.
      // See http://cmsdoc.cern.ch/cms/TRIDAS/horizontal/RUWG/DAQ_IF_guide/DAQ_IF_guide.html#CDF
      const uint32_t conscheck = trailer->conscheck;
      trailer->conscheck &= ~(FED_CRCS_MASK | 0x4);
      crcCalculator_.compute(crc,payload-sizeof(fedt_t),sizeof(fedt_t));
      trailer->conscheck = conscheck;

      const uint16_t trailerCRC = FED_CRCS_EXTRACT(conscheck);
      if ( trailerCRC != crc )
      {
        std::ostringstream oss;
        oss << "Wrong CRC checksum:" << std::hex;
        oss << " FED trailer claims 0x" << trailerCRC;
        oss << ", but recalculation gives 0x" << crc;
        XCEPT_RAISE(exception::DataCorruption, oss.str());
      }
    }

    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    typename InputMonitors::iterator pos = inputMonitors_.find(fedId);
    if ( pos == inputMonitors_.end() )
    {
      std::ostringstream msg;
      msg << "The received FED id " << fedId;
      msg << " is not in the excepted FED list: ";
      std::copy(configuration_->fedSourceIds.begin(), configuration_->fedSourceIds.end(),
        std::ostream_iterator<uint16_t>(msg," "));
      XCEPT_RAISE(exception::Configuration, msg.str());
    }

    ++(pos->second.perf.i2oCount);
    pos->second.perf.sumOfSizes += fedSize;
    pos->second.perf.sumOfSquares += fedSize*fedSize;
    pos->second.lastEventNumber = eventNumber;

    if ( ferolHeader->is_last_packet() )
      ++(pos->second.perf.logicalCount);
  }
  catch(exception::DataCorruption& e)
  {
    dumpFragmentToLogger(bufRef);

    std::ostringstream msg;
    msg << "Received a corrupted event " << eventNumber;
    msg << " from " << frame->PvtMessageFrame.StdMessageFrame.InitiatorAddress;
    XCEPT_RETHROW(exception::DataCorruption,msg.str(),e);
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::dumpFragmentToLogger(toolbox::mem::Reference* bufRef) const
{
  std::ostringstream oss;
  DumpUtility::dump(oss, bufRef);
  LOG4CPLUS_INFO(app_->getContext()->getLogger(), oss.str());
}


template<class Configuration>
bool evb::readoutunit::Input<Configuration>::getNextAvailableSuperFragment(FragmentChainPtr& superFragment)
{
  if ( ! handler_->getNextAvailableSuperFragment(superFragment) ) return false;

  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class Configuration>
bool evb::readoutunit::Input<Configuration>::getSuperFragmentWithEvBid(const EvBid& evbId, FragmentChainPtr& superFragment)
{
  if ( ! handler_->getSuperFragmentWithEvBid(evbId,superFragment) ) return false;

  updateSuperFragmentCounters(superFragment);

  return true;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::updateSuperFragmentCounters(const FragmentChainPtr& superFragment)
{
  boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

  const uint32_t size = superFragment->getSize();
  superFragmentMonitor_.lastEventNumber = superFragment->getEvBid().eventNumber();
  superFragmentMonitor_.perf.sumOfSizes += size;
  superFragmentMonitor_.perf.sumOfSquares += size*size;
  ++superFragmentMonitor_.perf.logicalCount;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::startProcessing(const uint32_t runNumber)
{
  resetMonitoringCounters();
  handler_->startProcessing(runNumber);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::drain()
{
  handler_->drain();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::stopProcessing()
{
  handler_->stopProcessing();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::appendMonitoringItems(InfoSpaceItems& items)
{
  eventRate_ = 0;
  superFragmentSize_ = 0;
  superFragmentSizeStdDev_ = 0;
  dataReadyCount_ = 0;

  items.add("eventRate", &eventRate_);
  items.add("superFragmentSize", &superFragmentSize_);
  items.add("superFragmentSizeStdDev", &superFragmentSizeStdDev_);
  items.add("dataReadyCount", &dataReadyCount_);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::updateMonitoringItems()
{
  uint32_t lastEventNumber = 0;
  uint32_t dataReadyCount = 0;

  {
    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    for (typename InputMonitors::iterator it = inputMonitors_.begin(), itEnd = inputMonitors_.end();
         it != itEnd; ++it)
    {
      lastEventNumber = it->second.lastEventNumber;
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

  eventRate_ = superFragmentMonitor_.rate;
  superFragmentSize_ = superFragmentMonitor_.eventSize;
  superFragmentSizeStdDev_ = superFragmentMonitor_.eventSizeStdDev;
  dataReadyCount_ = dataReadyCount;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    inputMonitors_.clear();
    xdata::Vector<xdata::UnsignedInteger32>::const_iterator it = configuration_->fedSourceIds.begin();
    const xdata::Vector<xdata::UnsignedInteger32>::const_iterator itEnd = configuration_->fedSourceIds.end();
    for ( ; it != itEnd ; ++it)
    {
      InputMonitoring monitor;
      monitor.lastEventNumber = 0;
      monitor.rate = 0;
      monitor.bandwidth = 0;
      inputMonitors_.insert(typename InputMonitors::value_type(it->value_,monitor));
    }
  }
  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    superFragmentMonitor_.lastEventNumber = 0;
    superFragmentMonitor_.rate = 0;
    superFragmentMonitor_.bandwidth = 0;
    superFragmentMonitor_.eventSize = 0;
    superFragmentMonitor_.eventSizeStdDev = 0;
    superFragmentMonitor_.perf.reset();
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::configure()
{
  if ( configuration_->blockSize % 8 != 0 )
  {
    XCEPT_RAISE(exception::Configuration, "The block size must be a multiple of 64-bits");
  }

  handler_->configure(configuration_);

  resetMonitoringCounters();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::printHtml(xgi::Output *out) const
{

  *out << "<div>"                                                 << std::endl;
  *out << "<p>Input - " << configuration_->inputSource.toString() <<"</p>" << std::endl;
  *out << "<table>"                                               << std::endl;

  const std::_Ios_Fmtflags originalFlags=out->flags();
  const int originalPrecision=out->precision();
  out->setf(std::ios::fixed);
  out->precision(2);

  {
    boost::mutex::scoped_lock sl(superFragmentMonitorMutex_);

    *out << "<tr>"                                                  << std::endl;
    *out << "<td>evt number of last super fragment</td>"            << std::endl;
    *out << "<td>" << superFragmentMonitor_.lastEventNumber << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << superFragmentMonitor_.bandwidth / 1e6 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    out->precision(4);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << superFragmentMonitor_.rate << "</td>"         << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>super fragment size (kB)</td>"                     << std::endl;
    *out << "<td>" << superFragmentMonitor_.eventSize / 1e3 << " +/- "
      << superFragmentMonitor_.eventSizeStdDev / 1e3 << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  handler_->printFragmentFIFOs(out,app_->getDescriptor()->getURN());

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Statistics per FED</th>"             << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>FED id</td>"                                       << std::endl;
  *out << "<td>Last event</td>"                                   << std::endl;
  *out << "<td>Size (kB)</td>"                                    << std::endl;
  *out << "<td>B/w (MB/s)</td>"                                   << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(inputMonitorsMutex_);

    typename InputMonitors::const_iterator it, itEnd;
    for (it=inputMonitors_.begin(), itEnd = inputMonitors_.end();
         it != itEnd; ++it)
    {
      *out << "<tr>"                                                << std::endl;
      *out << "<td>" << it->first << "</td>"                        << std::endl;
      *out << "<td>" << it->second.lastEventNumber << "</td>"       << std::endl;
      *out << "<td>" << it->second.eventSize / 1e3 << " +/- "
        << it->second.eventSizeStdDev / 1e3 << "</td>"              << std::endl;
      *out << "<td>" << it->second.bandwidth / 1e6 << "</td>"       << std::endl;
      *out << "</tr>"                                               << std::endl;
    }
  }

  out->flags(originalFlags);
  out->precision(originalPrecision);

  *out << "</table>"                                              << std::endl;

  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::printSuperFragmentFIFO(xgi::Output *out) const
{
  handler_->printSuperFragmentFIFO(out);
}


template<class Configuration>
evb::readoutunit::Input<Configuration>::FEROLproxy::FEROLproxy(Input<Configuration>* input) :
Handler(input),
superFragmentFIFO_("superFragmentFIFO")
{}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::configure(boost::shared_ptr<Configuration> configuration)
{
  this->doProcessing_ = false;
  dropInputData_ = configuration->dropInputData;

  superFragmentFIFO_.clear();
  superFragmentFIFO_.resize(configuration->fragmentFIFOCapacity);

  evbIdFactories_.clear();
  fragmentFIFOs_.clear();

  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint16_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      oss << "The fedSourceId " << fedId;
      oss << " is larger than maximal value FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      XCEPT_RAISE(exception::Configuration, oss.str());
    }

    std::ostringstream fifoName;
    fifoName << "fragmentFIFO_FED_" << fedId;
    FragmentFIFOPtr fragmentFIFO( new FragmentFIFO(fifoName.str()) );
    fragmentFIFO->resize(configuration->fragmentFIFOCapacity);
    fragmentFIFOs_.insert( typename FragmentFIFOs::value_type(fedId,fragmentFIFO) );
    evbIdFactories_.insert( typename EvBidFactories::value_type(fedId,EvBidFactory()) );
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::superFragmentReady
(
  toolbox::mem::Reference* bufRef
)
{
  I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  const unsigned char* payload = (unsigned char*)frame + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint16_t fedId = frame->fedid;
  const uint32_t eventNumber = frame->triggerno;
  const uint32_t lsNumber = fedId==GTP_FED_ID ? extractTriggerInformation(payload) : 0;

  const EvBid evbId = evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);

  FragmentChainPtr superFragment( new FragmentChain(evbId,bufRef) );

  if ( ! dropInputData_ )
    superFragmentFIFO_.enqWait(superFragment);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::rawDataAvailable
(
  toolbox::mem::Reference* bufRef,
  tcpla::MemoryCache* cache
)
{
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation() + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  ferolh_t* ferolHeader = (ferolh_t*)payload;
  assert( ferolHeader->signature() == FEROL_SIGNATURE );
  const uint16_t fedId = ferolHeader->fed_id();
  const uint32_t eventNumber = ferolHeader->event_number();

  const uint32_t lsNumber = fedId==GTP_FED_ID ? extractTriggerInformation(payload) : 0;

  // Input::checkEventFragment already checks that the fedId is known
  const EvBid evbId = evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);

  //std::cout << "**** got EvBid " << evbId << " from FED " << fedId << std::endl;
  //bufRef->release(); return;
  //cache->grantFrame(bufRef); return;

  FragmentChain::FragmentPtr fragment( new FragmentChain::Fragment(evbId,bufRef,cache) );

  if ( ! dropInputData_ )
    fragmentFIFOs_[fedId]->enqWait(fragment);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::startProcessing(const uint32_t runNumber)
{
  for (typename EvBidFactories::iterator it = evbIdFactories_.begin(), itEnd = evbIdFactories_.end();
        it != itEnd; ++it)
    it->second.reset(runNumber);

  this->doProcessing_ = true;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::drain()
{
  while ( ! superFragmentFIFO_.empty() ) ::usleep(1000);

  bool workDone;

  do
  {
    workDone = false;

    for (typename FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
         it != itEnd; ++it)
    {
      if ( ! it->second->empty() )
      {
        workDone = true;
        ::usleep(1000);
      }
    }
  } while ( workDone );
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::stopProcessing()
{
  this->doProcessing_ = false;

  superFragmentFIFO_.clear();

  for (typename FragmentFIFOs::iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    it->second->clear();
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::printSuperFragmentFIFO(xgi::Output* out) const
{
  superFragmentFIFO_.printVerticalHtml(out);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::printFragmentFIFOs
(
  xgi::Output* out,
  const toolbox::net::URN& urn
) const
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  superFragmentFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  for (typename FragmentFIFOs::const_iterator it = fragmentFIFOs_.begin(), itEnd = fragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\">"                                    << std::endl;
    it->second->printHtml(out, urn);
    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::DummyInputData::configure(boost::shared_ptr<Configuration> configuration)
{
  this->doProcessing_ = false;

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
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      oss << "The fedSourceId " << fedId;
      oss << " is larger than maximal value FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      XCEPT_RAISE(exception::Configuration, oss.str());
    }

    FragmentTrackerPtr fragmentTracker(
      new FragmentTracker(fedId,configuration->dummyFedSize,configuration->useLogNormal,
        configuration->dummyFedSizeStdDev,configuration->dummyFedSizeMin,configuration->dummyFedSizeMax,configuration->computeCRC)
    );
    fragmentTrackers_.insert( FragmentTrackers::value_type(fedId,fragmentTracker) );
  }
}


template<class Configuration>
bool evb::readoutunit::Input<Configuration>::DummyInputData::createSuperFragment
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

    for (uint16_t frame = 0; frame < frameCount; ++frame)
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
    this->input_->checkEventFragment(fragmentHead);
    superFragment->append(fragmentHead);
  }

  return superFragment->isValid();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::DummyInputData::startProcessing(const uint32_t runNumber)
{
  eventNumber_ = 0;
  evbIdFactory_.reset(runNumber);
  this->doProcessing_ = true;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::DummyInputData::stopProcessing()
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


  template <>
  inline void OneToOneQueue<readoutunit::FragmentChainPtr>::formatter
  (
    readoutunit::FragmentChainPtr fragmentChain,
    std::ostringstream* out
  ) const
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
  inline void OneToOneQueue<readoutunit::FragmentChain::FragmentPtr>::formatter
  (
    readoutunit::FragmentChain::FragmentPtr fragment,
    std::ostringstream* out
  ) const
  {
    if ( fragment.get() )
    {
      toolbox::mem::Reference* bufRef = fragment->bufRef;
      if ( bufRef )
      {
        I2O_DATA_READY_MESSAGE_FRAME* msg =
          (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
        *out << "I2O_DATA_READY_MESSAGE_FRAME:" << std::endl;
        *out << "  FED id: " << msg->fedid << std::endl;
        *out << "  trigger no: " << msg->triggerno << std::endl;
        *out << "  length: " << msg->partLength <<  "/" << msg->totalLength << std::endl;
        *out << "  evbId: " << fragment->evbId << std::endl;
      }
    }
    else
      *out << "n/a";
  }
}


#endif // _evb_readoutunit_Input_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
