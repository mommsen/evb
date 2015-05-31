#ifndef _evb_readoutunit_BUposter_h_
#define _evb_readoutunit_BUposter_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "cgicc/HTMLClasses.h"
#include "evb/OneToOneQueue.h"
#include "evb/readoutunit/StateMachine.h"
#include "i2o/utils/AddressMap.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xgi/Output.h"


namespace evb {

  namespace readoutunit { // namespace evb::readoutunit

    /**
     * \ingroup xdaqApps
     * \brief Singleton to post I2O messages to BU
     */

    template<class ReadoutUnit>
    class BUposter : public toolbox::lang::Class
    {

    public:

      BUposter(ReadoutUnit*);

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
       * Return the poster FIFOs as HTML snipped
       */
      cgicc::div getPosterFIFOs() const;


    private:

      void startPosterWorkLoop();
      bool postFrames(toolbox::task::WorkLoop*);

      ReadoutUnit* readoutUnit_;

      typedef OneToOneQueue<toolbox::mem::Reference*> FrameFIFO;
      typedef boost::shared_ptr<FrameFIFO> FrameFIFOPtr;
      struct BUdescriptorAndFIFO {
        xdaq::ApplicationDescriptor* bu;
        const FrameFIFOPtr frameFIFO;
        BUdescriptorAndFIFO(xdaq::ApplicationDescriptor* bu, const FrameFIFOPtr frameFIFO)
          : bu(bu),frameFIFO(frameFIFO) {};
      };
      typedef std::map<I2O_TID,BUdescriptorAndFIFO> BuFrameQueueMap;
      BuFrameQueueMap buFrameQueueMap_;
      mutable boost::mutex buFrameQueueMapMutex_;

      toolbox::task::WorkLoop* posterWL_;
      toolbox::task::ActionSignature* posterAction_;
      volatile bool doProcessing_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////


template<class ReadoutUnit>
evb::readoutunit::BUposter<ReadoutUnit>::BUposter(ReadoutUnit* readoutUnit) :
readoutUnit_(readoutUnit),
doProcessing_(false)
{
  startPosterWorkLoop();
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::startPosterWorkLoop()
{
  try
  {
    posterWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( readoutUnit_->getIdentifier("buPoster"), "waiting" );

    if ( ! posterWL_->isActive() )
    {
      posterAction_ =
        toolbox::task::bind(this, &evb::readoutunit::BUposter<ReadoutUnit>::postFrames,
                            readoutUnit_->getIdentifier("postFrames") );

      posterWL_->activate();
    }
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
    boost::mutex::scoped_lock sl(buFrameQueueMapMutex_);
    buFrameQueueMap_.clear();
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
    boost::mutex::scoped_lock sl(buFrameQueueMapMutex_);

    haveFrames = false;
    for (typename BuFrameQueueMap::const_iterator it = buFrameQueueMap_.begin(), itEnd = buFrameQueueMap_.end();
          it != itEnd; ++it)
    {
      haveFrames |= ( ! it->second.frameFIFO->empty() );
    }
  } while ( haveFrames && ::usleep(1000) );
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::stopProcessing()
{
  doProcessing_ = false;
}


template<class ReadoutUnit>
void evb::readoutunit::BUposter<ReadoutUnit>::sendFrame(const I2O_TID tid, toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(buFrameQueueMapMutex_);

  typename BuFrameQueueMap::iterator pos = buFrameQueueMap_.lower_bound(tid);
  if ( pos == buFrameQueueMap_.end() || buFrameQueueMap_.key_comp()(tid,pos->first) )
  {
    // new TID
    std::ostringstream name;
    name << "frameFIFO_BU" << tid;
    const FrameFIFOPtr frameFIFO( new FrameFIFO(readoutUnit_,name.str()) );
    frameFIFO->resize(readoutUnit_->getConfiguration()->fragmentRequestFIFOCapacity);

    try
    {
      BUdescriptorAndFIFO buDescriptorAndFIFO(
        i2o::utils::getAddressMap()->getApplicationDescriptor(tid),
        frameFIFO);

      pos = buFrameQueueMap_.insert(pos, typename BuFrameQueueMap::value_type(tid,buDescriptorAndFIFO));
    }
    catch(xcept::Exception& e)
    {
      std::ostringstream oss;
      oss << "Failed to get application descriptor for BU with tid ";
      oss << tid;
      XCEPT_RAISE(exception::I2O, oss.str());
    }
  }

  pos->second.frameFIFO->enqWait(bufRef);
}


template<class ReadoutUnit>
bool evb::readoutunit::BUposter<ReadoutUnit>::postFrames(toolbox::task::WorkLoop*)
{
  if ( ! doProcessing_ ) return false;

  bool workDone = false;
  toolbox::mem::Reference* bufRef;

  try
  {
    do {
      workDone = false;
      for (typename BuFrameQueueMap::const_iterator it = buFrameQueueMap_.begin();
           it != buFrameQueueMap_.end(); ++it)
      {
        if ( it->second.frameFIFO->deq(bufRef) )
        {
          try
          {
            readoutUnit_->getApplicationContext()->
              postFrame(
                bufRef,
                readoutUnit_->getApplicationDescriptor(),
                it->second.bu
              );
          }
          catch(xcept::Exception& e)
          {
            std::ostringstream oss;
            oss << "Failed to send super fragment to BU TID ";
            oss << it->first;
            XCEPT_RETHROW(exception::I2O, oss.str(), e);
          }
          workDone = true;
        }
      }
    } while ( doProcessing_ && workDone );

    ::usleep(100);
  }
  catch(xcept::Exception& e)
  {
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
  }
  catch(std::exception& e)
  {
    XCEPT_DECLARE(exception::I2O,
                  sentinelException, e.what());
    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }
  catch(...)
  {
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
  boost::mutex::scoped_lock sl(buFrameQueueMapMutex_);

  for (typename BuFrameQueueMap::const_iterator it = buFrameQueueMap_.begin(), itEnd = buFrameQueueMap_.end();
       it != itEnd; ++it)
  {
    div.add(it->second.frameFIFO->getHtmlSnipped());
  }

  return div;
}


#endif // _evb_readoutunit_BUposter_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
