#ifndef _evb_readoutunit_MetaDataStream_h_
#define _evb_readoutunit_MetaDataStream_h_

#include <stdint.h>
#include <string.h>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "cgicc/HTMLClasses.h"
#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/FerolStream.h"
#include "evb/readoutunit/MetaData.h"
#include "evb/readoutunit/MetaDataRetriever.h"
#include "evb/readoutunit/ReadoutUnit.h"
#include "evb/readoutunit/SuperFragment.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/ferol_header.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


namespace evb {

  namespace readoutunit {

   /**
    * \ingroup xdaqApps
    * \brief Represent a stream for SCAL data
    */

    template<class ReadoutUnit, class Configuration>
    class MetaDataStream : public FerolStream<ReadoutUnit,Configuration>, public toolbox::lang::Class

    {
    public:

      MetaDataStream(ReadoutUnit*,MetaDataRetrieverPtr);

      ~MetaDataStream();

      /**
       * Append the next FED fragment to the super fragment
       */
      virtual void appendFedFragment(SuperFragmentPtr&);

      /**
       * Start processing events
       */
      virtual void startProcessing(const uint32_t runNumber);

      /**
       * Return a CGI table row with statistics for this FED
       */
      virtual cgicc::tr getFedTableRow() const;

      /**
       * Return the content of the fragment FIFO as HTML snipped
       */
      virtual cgicc::div getHtmlSnippedForFragmentFIFO() const
      { return cgicc::div(); }


    private:

      void createMetaDataPool();
      void startRequestWorkLoop();
      void fillInitialData();
      toolbox::mem::Reference* getFedFragment(const uint32_t eventNumber);
      bool metaDataRequest(toolbox::task::WorkLoop*);

      toolbox::mem::Reference* currentDataBufRef_;
      toolbox::mem::Reference* nextDataBufRef_;
      mutable boost::mutex dataMutex_;

      toolbox::mem::Pool* fragmentPool_;
      toolbox::task::WorkLoop* metaDataRequestWorkLoop_;
      toolbox::task::ActionSignature* metaDataRequestAction_;

      MetaDataRetrieverPtr metaDataRetriever_;
      CRCCalculator crcCalculator_;
      const std::string subSystem_;

    };

  } } // namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit,class Configuration>
evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::MetaDataStream
(
  ReadoutUnit* readoutUnit,
  MetaDataRetrieverPtr metaDataRetriever
) :
  FerolStream<ReadoutUnit,Configuration>(readoutUnit,SOFT_FED_ID),
  currentDataBufRef_(0),
  nextDataBufRef_(0),
  metaDataRetriever_(metaDataRetriever),
  subSystem_("DAQ")
{
  createMetaDataPool();
  startRequestWorkLoop();
}


template<class ReadoutUnit,class Configuration>
evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::~MetaDataStream()
{
  if ( metaDataRequestWorkLoop_ && metaDataRequestWorkLoop_->isActive() )
    metaDataRequestWorkLoop_->cancel();
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::createMetaDataPool()
{
  toolbox::net::URN urn("toolbox-mem-pool", this->readoutUnit_->getIdentifier("metaDataFragmentPool"));

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
    toolbox::mem::HeapAllocator* a = new toolbox::mem::HeapAllocator();
    fragmentPool_ = toolbox::mem::getMemoryPoolFactory()->createPool(urn,a);
  }
  catch(toolbox::mem::exception::Exception& e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
                  "Failed to create memory pool for metaData fragments", e);
  }
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::startRequestWorkLoop()
{
  try
  {
    metaDataRequestWorkLoop_ =
      toolbox::task::getWorkLoopFactory()->getWorkLoop(this->readoutUnit_->getIdentifier("metaDataRequest"), "waiting");

    if ( !metaDataRequestWorkLoop_->isActive() )
      metaDataRequestWorkLoop_->activate();

    metaDataRequestAction_ =
      toolbox::task::bind(this,
                          &evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::metaDataRequest,
                          this->readoutUnit_->getIdentifier("metaDataRequestAction"));
  }
  catch(xcept::Exception& e)
  {
    std::string msg = "Failed to start metaData request workloop";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}



template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::fillInitialData()
{
  boost::mutex::scoped_lock sl(dataMutex_);

  nextDataBufRef_ = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,MetaData::dataSize);
  nextDataBufRef_->setDataSize(MetaData::dataSize);
  unsigned char* payload = (unsigned char*)nextDataBufRef_->getDataLocation();
  memset(payload,0,MetaData::dataSize);

  metaDataRetriever_->fillData(payload);

  if ( metaDataRetriever_->missingSubscriptions() )
  {
    std::ostringstream msg;
    msg << "Missing subscriptions from DIP:";

    metaDataRetriever_->addListOfSubscriptions(msg,true);

    LOG4CPLUS_ERROR(this->readoutUnit_->getApplicationLogger(), msg.str());

    XCEPT_DECLARE(exception::METADATA, sentinelException, msg.str());
    this->readoutUnit_->notifyQualified("error", sentinelException);
  }
}


template<class ReadoutUnit,class Configuration>
bool evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::metaDataRequest(toolbox::task::WorkLoop* wl)
{
  if ( ! this->doProcessing_ ) return false;

  metaDataRetriever_->subscribeToDip();

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,MetaData::dataSize);
  bufRef->setDataSize(MetaData::dataSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();

  // preserve the data from the previous iteration
  memcpy(payload,nextDataBufRef_->getDataLocation(),MetaData::dataSize);

  do {
    if ( metaDataRetriever_->fillData(payload) )
    {
      boost::mutex::scoped_lock sl(dataMutex_);

      if ( nextDataBufRef_ ) nextDataBufRef_->release();
      nextDataBufRef_ = bufRef;
      return true;
    }
    else
    {
      ::usleep(1000);
    }

  } while ( this->doProcessing_ );

  bufRef->release();
  if ( nextDataBufRef_ ) nextDataBufRef_->release();
  nextDataBufRef_ = 0;

  return false;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::appendFedFragment(SuperFragmentPtr& superFragment)
{
  if ( currentDataBufRef_ != nextDataBufRef_ )
  {
    boost::mutex::scoped_lock sl(dataMutex_);

    if ( currentDataBufRef_ ) currentDataBufRef_->release();
    currentDataBufRef_ = nextDataBufRef_->duplicate();
  }

  const EvBid& evbId = superFragment->getEvBid();

  toolbox::mem::Reference* bufRef = getFedFragment( evbId.eventNumber() );

  FedFragmentPtr fedFragment(
    new FedFragment(this->fedId_,
                    false,
                    subSystem_,
                    evbId,
                    bufRef)
  );

  this->updateInputMonitor(fedFragment);
  this->maybeDumpFragmentToFile(fedFragment);

  superFragment->append(fedFragment);
}


template<class ReadoutUnit,class Configuration>
toolbox::mem::Reference* evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::getFedFragment
(
  const uint32_t eventNumber
)
{
  const uint32_t dataSize = currentDataBufRef_->getDataSize();
  const uint32_t fedSize = dataSize + sizeof(fedh_t) + sizeof(fedt_t);
  const unsigned char* dataPos = (unsigned char*)currentDataBufRef_->getDataLocation();

  if ( (fedSize & 0x7) != 0 )
  {
    std::ostringstream msg;
    msg << "The SCAL FED " << this->fedId_ << " is " << fedSize << " Bytes, which is not a multiple of 8 Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,fedSize);

  bufRef->setDataSize(fedSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();

  fedh_t* fedHeader = (fedh_t*)payload;
  fedHeader->sourceid = this->fedId_ << FED_SOID_SHIFT;
  fedHeader->eventid = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | eventNumber;

  memcpy(payload+sizeof(fedh_t),dataPos,dataSize);

  fedt_t* fedTrailer = (fedt_t*)(payload + sizeof(fedh_t) + dataSize);
  fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) | (fedSize>>3);
  fedTrailer->conscheck = 0;
  const uint16_t crc = crcCalculator_.compute(payload,fedSize);
  fedTrailer->conscheck = (crc << FED_CRCS_SHIFT);

  return bufRef;
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::startProcessing(const uint32_t runNumber)
{
  FerolStream<ReadoutUnit,Configuration>::startProcessing(runNumber);

  if ( currentDataBufRef_ )
  {
    currentDataBufRef_->release();
    currentDataBufRef_ = 0;
  }

  fillInitialData();

  metaDataRequestWorkLoop_->submit(metaDataRequestAction_);
}


template<class ReadoutUnit,class Configuration>
cgicc::tr evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::getFedTableRow() const
{
  using namespace cgicc;
  const std::string fedId = boost::lexical_cast<std::string>(this->fedId_);

  tr row;
  row.add(td(fedId));
  row.add(td()
          .add(button("dump").set("type","button").set("title","write the next FED fragment to /tmp")
               .set("onclick","dumpFragments("+fedId+",1);")));
  {
    boost::mutex::scoped_lock sl(this->inputMonitorMutex_);

    row.add(td(boost::lexical_cast<std::string>(this->inputMonitor_.lastEventNumber)));
    row.add(td(boost::lexical_cast<std::string>(static_cast<uint32_t>(this->inputMonitor_.eventSize))
               +" +/- "+boost::lexical_cast<std::string>(static_cast<uint32_t>(this->inputMonitor_.eventSizeStdDev))));
    row.add(td(doubleToString(this->inputMonitor_.throughput / 1e6,1)));
  }

  row.add(metaDataRetriever_->getDipStatus( this->readoutUnit_->getURN().toString() ));

  return row;
}


#endif // _evb_readoutunit_MetaDataStream_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
