#ifndef _evb_readoutunit_Input_h_
#define _evb_readoutunit_Input_h_

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <iterator>
#include <map>
#include <stdint.h>
#include <string>

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
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();
      
      /**
       * Configure
       */
      void configure();
      
      /**
       * Start processing messages
       */
      void startProcessing(const uint32_t runNumber);
      
      /**
       * Stop processing messages
       */
      void stopProcessing();
      
      /**
       * Remove all data
       */
      void clear();
      
      /**
       * Print monitoring information as HTML snipped
       */
      void printHtml(xgi::Output*);
      
      /**
       * Print the content of the super-fragment FIFO as HTML snipped
       */
      void printSuperFragmentFIFO(xgi::Output* out);
 
      
    protected:
      
      class Handler
      {
      public:
        
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
        virtual void stopProcessing() {};
        virtual void clear() {};
        virtual void printSuperFragmentFIFO(xgi::Output*) {};
        virtual void printSuperFragmentFIFOsnipped(xgi::Output*, const toolbox::net::URN&) {};
        
      };
           
      class FEROLproxy : public Handler
      {
      public:
        
        FEROLproxy();

        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void superFragmentReady(toolbox::mem::Reference*);
        virtual void rawDataAvailable(toolbox::mem::Reference*, tcpla::MemoryCache*);
        virtual void startProcessing(const uint32_t runNumber);
        virtual void clear();
        virtual void printSuperFragmentFIFO(xgi::Output*);
        virtual void printSuperFragmentFIFOsnipped(xgi::Output*, const toolbox::net::URN&);
        
      protected:
        
        virtual uint32_t extractTriggerInformation(const unsigned char*) const
        { return 0; }

        typedef OneToOneQueue<FragmentChainPtr> SuperFragmentFIFO;
        SuperFragmentFIFO superFragmentFIFO_;
        
        typedef std::map<EvBid,FragmentChainPtr> SuperFragmentMap;
        SuperFragmentMap superFragmentMap_;
        boost::shared_mutex superFragmentMapMutex_;
        
      private:
        
        void addFragment(toolbox::mem::Reference*);
        toolbox::mem::Reference* copyDataIntoDataBlock(FragmentChainPtr);
        void fillBlockInfo(toolbox::mem::Reference*, const EvBid&, const uint32_t nbBlocks) const;
        
        bool dropInputData_;
        
        FragmentChain::ResourceList fedList_;
        
        typedef std::map<uint16_t,EvBidFactory> EvBidFactories;
        EvBidFactories evbIdFactories_;
        
      };

      class DummyInputData : public Handler
      {
      public:
        
        DummyInputData(Input<Configuration>* input) : input_(input) {};
        virtual void configure(boost::shared_ptr<Configuration>);
        virtual void startProcessing(const uint32_t runNumber);
        
      protected:
        
        bool createSuperFragment(const EvBid&, FragmentChainPtr&);

        EvBidFactory evbIdFactory_;
        uint32_t eventNumber_;

      private:
        
        Input<Configuration>* input_;
        FragmentChain::ResourceList fedList_;
        typedef std::map<uint16_t,FragmentTracker> FragmentTrackers;
        FragmentTrackers fragmentTrackers_;
        toolbox::mem::Pool* fragmentPool_;
      };

      virtual void getHandlerForInputSource(boost::shared_ptr<Handler>& handler)
      { handler.reset(new Handler); }
      
      const boost::shared_ptr<Configuration> configuration_;
      
    private:
      
      void dumpFragmentToLogger(toolbox::mem::Reference*) const;
      void updateInputCounters(toolbox::mem::Reference*);
      void updateSuperFragmentCounters(const FragmentChainPtr&);
      
      xdaq::ApplicationStub* app_;
      boost::shared_ptr<Handler> handler_;
      
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
      boost::mutex inputMonitorsMutex_;
      
      InputMonitoring superFragmentMonitor_;
      boost::mutex superFragmentMonitorMutex_;
      
      bool acceptI2Omessages_;
      xdata::UnsignedInteger32 lastEventNumberFromFEROLs_;
      xdata::UnsignedInteger64 i2oDataReadyCount_;
      
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
handler_(new Handler()),
acceptI2Omessages_(false)
{}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::inputSourceChanged()
{
  getHandlerForInputSource(handler_);
  handler_->configure(configuration_);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::superFragmentReady(toolbox::mem::Reference* bufRef)
{
  if ( acceptI2Omessages_ )
  {
    updateInputCounters(bufRef);
    dumpFragmentToLogger(bufRef);
    handler_->superFragmentReady(bufRef);
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::rawDataAvailable(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
{
  if ( acceptI2Omessages_ )
  {
    updateInputCounters(bufRef);
    dumpFragmentToLogger(bufRef);
    handler_->rawDataAvailable(bufRef,cache);
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::updateInputCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(inputMonitorsMutex_);
  
  const unsigned char* payload = (unsigned char*)bufRef->getDataLocation()
    + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const ferolh_t* ferolHeader = (ferolh_t*)payload;
  assert( ferolHeader->signature() == FEROL_SIGNATURE );
  const uint16_t fedId = ferolHeader->fed_id();
  const uint32_t eventNumber = ferolHeader->event_number();
  const uint32_t fedSize = ferolHeader->data_length();
  
  // I2O_DATA_READY_MESSAGE_FRAME* frame =
  //   (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  // const uint16_t fedId = frame->fedid;
  // const uint32_t eventNumber = frame->triggerno;
  // const uint32_t fedSize = frame->totalLength;
  
  typename InputMonitors::iterator pos = inputMonitors_.find(fedId);
  if ( pos == inputMonitors_.end() )
  {
    std::ostringstream msg;
    msg << "The received FED id " << fedId;
    msg << " for event " << eventNumber;
    msg << " is not in the excepted FED list: ";
    std::copy(configuration_->fedSourceIds.begin(), configuration_->fedSourceIds.end(),
      std::ostream_iterator<uint16_t>(msg," "));
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  
  pos->second.lastEventNumber = eventNumber;
  pos->second.perf.sumOfSizes += fedSize;
  pos->second.perf.sumOfSquares += fedSize*fedSize;
  ++(pos->second.perf.i2oCount);
  //if ( ferolHeader->is_last_packet() )
  ++(pos->second.perf.logicalCount);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::dumpFragmentToLogger(toolbox::mem::Reference* bufRef) const
{
  if ( ! configuration_->dumpFragmentsToLogger.value_ ) return;

  std::stringstream oss;
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
  handler_->startProcessing(runNumber);
  acceptI2Omessages_ = true;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::stopProcessing()
{
  acceptI2Omessages_ = false;
  handler_->stopProcessing();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::clear()
{
  acceptI2Omessages_ = false;
  handler_->clear();
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::appendMonitoringItems(InfoSpaceItems& items)
{
  lastEventNumberFromFEROLs_ = 0;
  i2oDataReadyCount_ = 0;

  items.add("lastEventNumberFromFEROLs", &lastEventNumberFromFEROLs_);
  items.add("i2oDataReadyCount", &i2oDataReadyCount_);
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
      if ( lastEventNumber < it->second.lastEventNumber )
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
  
  lastEventNumberFromFEROLs_ = lastEventNumber;
  i2oDataReadyCount_ = dataReadyCount;
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
  handler_->configure(configuration_);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::printHtml(xgi::Output *out)
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
    out->precision(2);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>super fragment size (kB)</td>"                     << std::endl;
    *out << "<td>" << superFragmentMonitor_.eventSize / 1e3 << " +/- "
      << superFragmentMonitor_.eventSizeStdDev / 1e3 << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  handler_->printSuperFragmentFIFOsnipped(out,app_->getDescriptor()->getURN());  
  
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
void evb::readoutunit::Input<Configuration>::printSuperFragmentFIFO(xgi::Output *out)
{
  handler_->printSuperFragmentFIFO(out);
}


template<class Configuration>
evb::readoutunit::Input<Configuration>::FEROLproxy::FEROLproxy() :
superFragmentFIFO_("superFragmentFIFO")
{}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::configure(boost::shared_ptr<Configuration> configuration)
{
  dropInputData_ = configuration->dropInputData;

  clear();
  
  superFragmentFIFO_.resize(configuration->superFragmentFIFOCapacity);
  
  evbIdFactories_.clear();
  fedList_.clear();
  fedList_.reserve(configuration->fedSourceIds.size());
  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint16_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      
      oss << "fedSourceId is too large.";
      oss << "Actual value: " << fedId;
      oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    fedList_.push_back(fedId);
    evbIdFactories_.insert(EvBidFactories::value_type(fedId,EvBidFactory()));
  }
  std::sort(fedList_.begin(),fedList_.end());
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::superFragmentReady(toolbox::mem::Reference* bufRef)
{
  I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  const unsigned char* payload = (unsigned char*)frame + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint16_t fedId = frame->fedid;
  const uint32_t eventNumber = frame->triggerno;
  const uint32_t lsNumber = fedId==GTP_FED_ID ? extractTriggerInformation(payload) : 0;
  
  const EvBid evbId = evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
  
  FragmentChainPtr superFragment( new FragmentChain(evbId,bufRef) );
  
  // boost::unique_lock<boost::shared_mutex> uniqueLock(superFragmentMapMutex_);
  // if ( ! superFragmentMap_.insert(SuperFragmentMap::value_type(evbId,superFragment)).second )
  // {
  //   std::ostringstream msg;
  //   msg << "Received a duplicated event with EvB id " << evbId;
  //   XCEPT_RAISE(exception::EventOrder, msg.str());
  // }
  
  if ( ! dropInputData_ )
    while( ! superFragmentFIFO_.enq(superFragment) ) ::usleep(1000);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::rawDataAvailable(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
{
  boost::upgrade_lock<boost::shared_mutex> upgradeLock(superFragmentMapMutex_);
  
  I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  unsigned char* payload = (unsigned char*)frame + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  ferolh_t* ferolHeader = (ferolh_t*)payload;
  assert( ferolHeader->signature() == FEROL_SIGNATURE );
  const uint16_t fedId = ferolHeader->fed_id();
  const uint32_t eventNumber = ferolHeader->event_number();
  
  const uint32_t lsNumber = fedId==GTP_FED_ID ? extractTriggerInformation(payload) : 0;

  const EvBid evbId = evbIdFactories_[fedId].getEvBid(eventNumber,lsNumber);
  //std::cout << "**** got EvBid " << evbId << " from FED " << fedId << std::endl;
  //bufRef->release(); return;
  //cache->grantFrame(bufRef); return;
  
  SuperFragmentMap::iterator fragmentPos = superFragmentMap_.lower_bound(evbId);
  if ( fragmentPos == superFragmentMap_.end() || (superFragmentMap_.key_comp()(evbId,fragmentPos->first)) )
  {
    // new super-fragment
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
    
    FragmentChainPtr superFragment( new FragmentChain(evbId,fedList_) );
    fragmentPos = superFragmentMap_.insert(fragmentPos, SuperFragmentMap::value_type(evbId,superFragment));
  }
  
  if ( ! fragmentPos->second->append(fedId,bufRef,cache) )
  {
    std::ostringstream msg;
    msg << "Received a duplicated FED id " << fedId
      << "for event " << eventNumber;
    XCEPT_RAISE(exception::EventOrder, msg.str());
  }
  
  if ( fragmentPos->second->isComplete() )
  {
    if ( dropInputData_ )
      superFragmentMap_.erase(fragmentPos);
  }
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::startProcessing(const uint32_t runNumber)
{
  for ( EvBidFactories::iterator it = evbIdFactories_.begin(), itEnd = evbIdFactories_.end();
        it != itEnd; ++it)
    it->second.reset(runNumber);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::clear()
{
  boost::unique_lock<boost::shared_mutex> sl(superFragmentMapMutex_);
  
  superFragmentMap_.clear();

  FragmentChainPtr superFragment;
  while ( superFragmentFIFO_.deq(superFragment) ) {};
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::printSuperFragmentFIFO(xgi::Output* out)
{
  superFragmentFIFO_.printVerticalHtml(out);
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::FEROLproxy::printSuperFragmentFIFOsnipped(xgi::Output* out, const toolbox::net::URN& urn)
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  superFragmentFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::DummyInputData::configure(boost::shared_ptr<Configuration> configuration)
{
  clear();
  
  toolbox::net::URN urn("toolbox-mem-pool", "FragmentPool");
  try
  {
    toolbox::mem::getMemoryPoolFactory()->destroyPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    // don't care
  }
  
  try
  {
    toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator(configuration->fragmentPoolSize.value_);
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for dummy fragments.", e);
  }
  
  fragmentTrackers_.clear();
  fedList_.clear();
  fedList_.reserve(configuration->fedSourceIds.size());
  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = configuration->fedSourceIds.begin(), itEnd = configuration->fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint16_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      
      oss << "fedSourceId is too large.";
      oss << "Actual value: " << fedId;
      oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    fedList_.push_back(fedId);
    fragmentTrackers_.insert(FragmentTrackers::value_type(fedId,
        FragmentTracker(fedId,configuration->dummyFedSize.value_,configuration->dummyFedSizeStdDev.value_)));
  }
}


template<class Configuration>
bool evb::readoutunit::Input<Configuration>::DummyInputData::createSuperFragment(const EvBid& evbId, FragmentChainPtr& superFragment)
{
  superFragment.reset( new FragmentChain(evbId, fedList_) );
  
  toolbox::mem::Reference* bufRef = 0;
  const uint32_t ferolBlockSize = 4*1024;
  
  for ( FragmentTrackers::iterator it = fragmentTrackers_.begin(), itEnd = fragmentTrackers_.end();
        it != itEnd; ++it)
  {
    uint32_t remainingFedSize = it->second.startFragment(evbId.eventNumber());
    const uint32_t frameSize = remainingFedSize+sizeof(I2O_DATA_READY_MESSAGE_FRAME)+sizeof(ferolh_t);
    uint32_t packetNumber = 0;
    
    try
    {
      bufRef = toolbox::mem::getMemoryPoolFactory()->
        getFrame(fragmentPool_,frameSize);
    }
    catch(xcept::Exception& e)
    {
      return false;
    }
    
    bufRef->setDataSize(frameSize);
    bzero(bufRef->getDataLocation(), bufRef->getBuffer()->getSize());
    I2O_DATA_READY_MESSAGE_FRAME* dataReadyMsg =
      (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
    dataReadyMsg->totalLength = remainingFedSize+sizeof(ferolh_t);
    dataReadyMsg->partLength = remainingFedSize+sizeof(ferolh_t);
    dataReadyMsg->fedid = it->first;
    dataReadyMsg->triggerno = evbId.eventNumber();
    
    unsigned char* frame = (unsigned char*)dataReadyMsg
      + sizeof(I2O_DATA_READY_MESSAGE_FRAME);
    
    while ( remainingFedSize > 0 )
    {
      assert( (remainingFedSize & 0x7) == 0 ); //must be a multiple of 8 Bytes
      uint32_t length;
      
      ferolh_t* ferolHeader = (ferolh_t*)frame;
      ferolHeader->set_signature();
      ferolHeader->set_packet_number(packetNumber);
      
      if (packetNumber == 0)
        ferolHeader->set_first_packet();
      
      if ( remainingFedSize > ferolBlockSize )
      {
        length = ferolBlockSize;
      }
      else
      {
        length = remainingFedSize;
        ferolHeader->set_last_packet();
      }
      remainingFedSize -= length;
      frame += sizeof(ferolh_t);
      
      const size_t filledBytes = it->second.fillData(frame, length);
      
      ferolHeader->set_data_length(filledBytes);
      ferolHeader->set_fed_id(it->first);
      ferolHeader->set_event_number(evbId.eventNumber());
      
      frame += filledBytes;
      
      ++packetNumber;
      assert(packetNumber < 2048);
    }
    
    input_->updateInputCounters(bufRef);
    superFragment->append(it->first,bufRef);
  }

  return true;
}


template<class Configuration>
void evb::readoutunit::Input<Configuration>::DummyInputData::startProcessing(const uint32_t runNumber)
{
  eventNumber_ = 0;
  evbIdFactory_.reset(runNumber);
}

namespace evb
{
  template <>
  inline void OneToOneQueue<evb::readoutunit::FragmentChainPtr>::formatter(readoutunit::FragmentChainPtr fragmentChain, std::ostringstream* out)
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
}


#endif // _evb_readoutunit_Input_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
