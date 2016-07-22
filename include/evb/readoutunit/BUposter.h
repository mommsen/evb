#ifndef _evb_readoutunit_BUposter_h_
#define _evb_readoutunit_BUposter_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <map>
#include <stdint.h>

#include "cgicc/HTMLClasses.h"
#include "evb/Constants.h"
#include "evb/OneToOneQueue.h"
#include "evb/PerformanceMonitor.h"
#include "evb/readoutunit/StateMachine.h"
#include "i2o/utils/AddressMap.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdata/Double.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace evb {

  namespace readoutunit { // namespace evb::readoutunit

    /**
     * \ingroup xdaqApps
     * \brief Post I2O messages to BU
     */

    template<class ReadoutUnit>
    class BUposter : public toolbox::lang::Class
    {

    public:

      BUposter(ReadoutUnit*);

      ~BUposter();

      /**
       * Send the bufRef to the BU
       */
      void sendFrame(const I2O_TID,toolbox::mem::Reference*);

      /**
       * Start processing events
       */
      void startProcessing();

      /**
       * Drain events
       */
      void drain() const;

      /**
       * Stop processing events
       */
      void stopProcessing();

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
       * Return the poster FIFOs as HTML snipped
       */
      cgicc::div getPosterFIFOs() const;

      /**
       * Return a cgicc::table containing information for each BU connection
       */
      cgicc::table getStatisticsPerBU() const;


    private:

      void startPosterWorkLoop();
      bool postFrames(toolbox::task::WorkLoop*);

      ReadoutUnit* readoutUnit_;

      typedef OneToOneQueue<toolbox::mem::Reference*> FrameFIFO;
      typedef boost::shared_ptr<FrameFIFO> FrameFIFOPtr;
      struct BUconnection {
        const I2O_TID tid;
        xdaq::ApplicationDescriptor* bu;
        const FrameFIFOPtr frameFIFO;
        uint64_t throughput;
        uint32_t i2oRate;
        double retryRate;
        PerformanceMonitor perf;
        mutable boost::mutex perfMutex;

        BUconnection(const I2O_TID tid, const FrameFIFOPtr& frameFIFO);
      };
      typedef boost::shared_ptr<BUconnection> BUconnectionPtr;
      typedef std::map<I2O_TID,BUconnectionPtr> BUconnections;
      BUconnections buConnections_;
      mutable boost::shared_mutex buConnectionsMutex_;

      toolbox::task::WorkLoop* posterWL_;
      toolbox::task::ActionSignature* posterAction_;
      volatile bool doProcessing_;
      volatile bool active_;

      xdata::Vector<xdata::UnsignedInteger32> buTids_;
      xdata::Vector<xdata::UnsignedInteger64> throughputPerBU_;
      xdata::Vector<xdata::UnsignedInteger32> fragmentRatePerBU_;
      xdata::Vector<xdata::Double> retryRatePerBU_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////


template<class ReadoutUnit>
evb::readoutunit::BUposter<ReadoutUnit>::BUposter(ReadoutUnit* readoutUnit) :
readoutUnit_(readoutUnit),
doProcessing_(false),
active_(false)
{
  startPosterWorkLoop();
}


template<class ReadoutUnit>
evb::readoutunit::BUposter<ReadoutUnit>::~BUposter()
{
  if ( posterWL_ && posterWL_->isActive() )
    posterWL_->cancel();
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::appendMonitoringItems(InfoSpaceItems& items)
{
  buTids_.clear();
  throughputPerBU_.clear();
  fragmentRatePerBU_.clear();
  retryRatePerBU_.clear();

  items.add("buTids", &buTids_);
  items.add("throughputPerBU", &throughputPerBU_);
  items.add("fragmentRatePerBU", &fragmentRatePerBU_);
  items.add("retryRatePerBU", &retryRatePerBU_);
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::updateMonitoringItems()
{
  boost::shared_lock<boost::shared_mutex> sl(buConnectionsMutex_);

  const uint32_t nbConnections = buConnections_.size();
  buTids_.clear();
  buTids_.reserve(nbConnections);

  throughputPerBU_.clear();
  throughputPerBU_.reserve(nbConnections);

  fragmentRatePerBU_.clear();
  fragmentRatePerBU_.reserve(nbConnections);

  retryRatePerBU_.clear();
  retryRatePerBU_.reserve(nbConnections);

  for (typename BUconnections::iterator it = buConnections_.begin(), itEnd = buConnections_.end();
       it != itEnd; ++it)
  {
    boost::mutex::scoped_lock sl(it->second->perfMutex);

    const double deltaT = it->second->perf.deltaT();
    it->second->throughput = it->second->perf.throughput(deltaT);
    it->second->i2oRate = it->second->perf.i2oRate(deltaT);
    it->second->retryRate = it->second->perf.retryRate(deltaT);
    it->second->perf.reset();

    buTids_.push_back(it->first);
    throughputPerBU_.push_back(it->second->throughput);
    fragmentRatePerBU_.push_back(it->second->i2oRate);
    retryRatePerBU_.push_back(it->second->retryRate);
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::startPosterWorkLoop()
{
  try
  {
    posterWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( readoutUnit_->getIdentifier("buPoster"), "waiting" );

    posterAction_ =
      toolbox::task::bind(this, &evb::readoutunit::BUposter<ReadoutUnit>::postFrames,
                          readoutUnit_->getIdentifier("postFrames") );

    if ( ! posterWL_->isActive() )
      posterWL_->activate();
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'buPoster'";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::startProcessing()
{
  {
    boost::unique_lock<boost::shared_mutex> ul(buConnectionsMutex_);
    buConnections_.clear();
  }

  doProcessing_ = true;
  posterWL_->submit(posterAction_);
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::drain() const
{
  bool haveFrames = false;
  do
  {
    boost::shared_lock<boost::shared_mutex> sl(buConnectionsMutex_);

    haveFrames = false;
    for (typename BUconnections::const_iterator it = buConnections_.begin(), itEnd = buConnections_.end();
          it != itEnd; ++it)
    {
      haveFrames |= ( ! it->second->frameFIFO->empty() );
    }
  } while ( haveFrames && ::usleep(1000) );
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::stopProcessing()
{
  doProcessing_ = false;
  while (active_) ::usleep(1000);

  {
    boost::unique_lock<boost::shared_mutex> ul(buConnectionsMutex_);

    for (typename BUconnections::const_iterator it = buConnections_.begin(), itEnd = buConnections_.end();
         it != itEnd; ++it)
    {
      toolbox::mem::Reference* bufRef;
      while ( it->second->frameFIFO->deq(bufRef) )
        bufRef->release();
    }
  }
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::sendFrame(const I2O_TID tid, toolbox::mem::Reference* bufRef)
{
  boost::upgrade_lock<boost::shared_mutex> sl(buConnectionsMutex_);

  typename BUconnections::iterator pos = buConnections_.lower_bound(tid);
  if ( pos == buConnections_.end() || buConnections_.key_comp()(tid,pos->first) )
  {
    // new TID
    boost::upgrade_to_unique_lock<boost::shared_mutex> ul(sl);

    std::ostringstream name;
    name << "frameFIFO_BU" << tid;
    const FrameFIFOPtr frameFIFO( new FrameFIFO(readoutUnit_,name.str()) );
    frameFIFO->resize(readoutUnit_->getConfiguration()->fragmentRequestFIFOCapacity);

    BUconnectionPtr buConnection(new BUconnection(tid,frameFIFO));
    pos = buConnections_.insert(pos, typename BUconnections::value_type(tid,buConnection));
  }

  pos->second->frameFIFO->enqWait(bufRef);
}


template<class ReadoutUnit>
bool evb::readoutunit::BUposter<ReadoutUnit>::postFrames(toolbox::task::WorkLoop*)
{
  if ( ! doProcessing_ ) return false;

  bool workDone = false;
  toolbox::mem::Reference* bufRef;

  try
  {
    active_ = true;
    do {
      boost::shared_lock<boost::shared_mutex> sl(buConnectionsMutex_);

      workDone = false;
      for (typename BUconnections::iterator it = buConnections_.begin();
           it != buConnections_.end(); ++it)
      {
        if ( it->second->frameFIFO->deq(bufRef) )
        {
          try
          {
            const uint32_t payloadSize = bufRef->getDataSize();
            const uint32_t retries = readoutUnit_->postMessage(bufRef,it->second->bu);
            {
              boost::mutex::scoped_lock sl(it->second->perfMutex);
              it->second->perf.retryCount += retries;
              it->second->perf.sumOfSizes += payloadSize;
              it->second->perf.sumOfSquares += payloadSize*payloadSize;
              it->second->perf.i2oCount++;
            }
          }
          catch(exception::I2O& e)
          {
            std::ostringstream msg;
            msg << "Failed to send super fragment to BU TID ";
            msg << it->first;
            XCEPT_RETHROW(exception::I2O, msg.str(), e);
          }
          workDone = true;
        }
      }
    } while ( doProcessing_ && workDone );

    active_ = false;
    ::usleep(100);
  }
  catch(xcept::Exception& e)
  {
    active_ = false;
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    active_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    active_ = false;
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, "unkown exception");
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  return doProcessing_;
}


template<class ReadoutUnit>
cgicc::div evb::readoutunit::BUposter<ReadoutUnit>::getPosterFIFOs() const
{
  using namespace cgicc;

  cgicc::div div;
  boost::shared_lock<boost::shared_mutex> sl(buConnectionsMutex_);

  for (typename BUconnections::const_iterator it = buConnections_.begin(), itEnd = buConnections_.end();
       it != itEnd; ++it)
  {
    div.add(it->second->frameFIFO->getHtmlSnipped());
  }

  return div;
}


template<class ReadoutUnit>
cgicc::table evb::readoutunit::BUposter<ReadoutUnit>::getStatisticsPerBU() const
{
  using namespace cgicc;

  table table;
  table.set("title","Statistics per BU connection");

  table.add(tr()
            .add(th("Statistics per BU").set("colspan","5")));
  table.add(tr()
            .add(td("Instance"))
            .add(td("TID"))
            .add(td("Throughput (MB/s)"))
            .add(td("I2O rate (Hz)"))
            .add(td("Retry rate (Hz)")));

  boost::shared_lock<boost::shared_mutex> sl(buConnectionsMutex_);

  for (typename BUconnections::const_iterator it = buConnections_.begin(), itEnd = buConnections_.end();
       it != itEnd; ++it)
  {
    xdaq::ApplicationDescriptor* bu = i2o::utils::getAddressMap()->getApplicationDescriptor(it->first);
    const std::string url = bu->getContextDescriptor()->getURL() + "/" + bu->getURN();
    table.add(tr()
              .add(td()
                   .add(a("BU "+boost::lexical_cast<std::string>(bu->getInstance())).set("href",url).set("target","_blank")))
              .add(td(boost::lexical_cast<std::string>(it->first)))
              .add(td(doubleToString(it->second->throughput / 1e6,2)))
              .add(td(boost::lexical_cast<std::string>(it->second->i2oRate)))
              .add(td(doubleToString(it->second->retryRate,2))));
  }

  return table;
}


template<class ReadoutUnit>
evb::readoutunit::BUposter<ReadoutUnit>::BUconnection::BUconnection
(
  const I2O_TID tid,
  const FrameFIFOPtr& frameFIFO
)
  : tid(tid),frameFIFO(frameFIFO)
{
  try
  {
    bu = i2o::utils::getAddressMap()->getApplicationDescriptor(tid);
  }
  catch(xcept::Exception& e)
  {
    std::ostringstream msg;
    msg << "Failed to get application descriptor for BU with tid ";
    msg << tid;
    XCEPT_RAISE(exception::I2O, msg.str());
  }
}


#endif // _evb_readoutunit_BUposter_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
