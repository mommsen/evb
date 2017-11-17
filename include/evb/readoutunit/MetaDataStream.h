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
      void getFedFragment(const EvBid&, FedFragmentPtr&);
      bool metaDataRequest(toolbox::task::WorkLoop*);

      toolbox::mem::Reference* currentDataBufRef_;
      toolbox::mem::Reference* nextDataBufRef_;
      mutable boost::mutex dataMutex_;

      toolbox::mem::Pool* fragmentPool_;
      toolbox::task::WorkLoop* metaDataRequestWorkLoop_;
      toolbox::task::ActionSignature* metaDataRequestAction_;

      MetaDataRetrieverPtr metaDataRetriever_;
      CRCCalculator crcCalculator_;

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
  metaDataRetriever_(metaDataRetriever)
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
bool evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::metaDataRequest(toolbox::task::WorkLoop* wl)
{
  if ( ! this->doProcessing_ ) return false;

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,MetaData::dataSize);
  bufRef->setDataSize(MetaData::dataSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();

  if ( nextDataBufRef_ )
  {
    // preserve the data from the previous iteration
    memcpy(payload,nextDataBufRef_->getDataLocation(),MetaData::dataSize);
  }
  else
  {
    memset(payload,0,MetaData::dataSize);
  }

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
  const EvBid& evbId = superFragment->getEvBid();

  if ( currentDataBufRef_ != nextDataBufRef_ )
  {
    boost::mutex::scoped_lock sl(dataMutex_);

    if ( currentDataBufRef_ ) currentDataBufRef_->release();
    currentDataBufRef_ = nextDataBufRef_->duplicate();
  }

  if ( ! currentDataBufRef_ )
  {
    XCEPT_DECLARE(exception::SCAL,
                  sentinelException, "Cannot retrieve any metaData information");
    this->readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  FedFragmentPtr fedFragment;
  getFedFragment(evbId,fedFragment);
  this->updateInputMonitor(fedFragment);
  this->maybeDumpFragmentToFile(fedFragment);

  superFragment->append(fedFragment);
}


template<class ReadoutUnit,class Configuration>
void evb::readoutunit::MetaDataStream<ReadoutUnit,Configuration>::getFedFragment
(
  const EvBid& evbId,
  FedFragmentPtr& fedFragment
)
{
  const uint32_t eventNumber = evbId.eventNumber();
  const uint32_t ferolPayloadSize = FEROL_BLOCK_SIZE - sizeof(ferolh_t);
  const uint32_t dataSize = currentDataBufRef_->getDataSize();
  const uint32_t fedSize = dataSize + sizeof(fedh_t) + sizeof(fedt_t);
  const unsigned char* dataPos = (unsigned char*)currentDataBufRef_->getDataLocation();

  if ( (fedSize & 0x7) != 0 )
  {
    std::ostringstream msg;
    msg << "The SCAL FED " << this->fedId_ << " is " << fedSize << " Bytes, which is not a multiple of 8 Bytes";
    XCEPT_RAISE(exception::Configuration, msg.str());
  }

  // ceil(x/y) can be expressed as (x+y-1)/y for positive integers
  const uint16_t ferolBlocks = (fedSize + ferolPayloadSize - 1)/ferolPayloadSize;
  assert(ferolBlocks < 2048);
  const uint32_t ferolSize = fedSize + ferolBlocks*sizeof(ferolh_t);
  uint32_t dataOffset = 0;
  uint16_t crc(0xffff);

  toolbox::mem::Reference* bufRef = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fragmentPool_,ferolSize);

  bufRef->setDataSize(ferolSize);
  unsigned char* payload = (unsigned char*)bufRef->getDataLocation();
  memset(payload, 0, ferolSize);

  for (uint16_t packetNumber = 0; packetNumber < ferolBlocks; ++packetNumber)
  {
    ferolh_t* ferolHeader = (ferolh_t*)payload;
    ferolHeader->set_signature();
    ferolHeader->set_fed_id(this->fedId_);
    ferolHeader->set_event_number(eventNumber);
    ferolHeader->set_packet_number(packetNumber);
    payload += sizeof(ferolh_t);
    uint32_t dataLength = 0;

    if (packetNumber == 0)
    {
      ferolHeader->set_first_packet();
      fedh_t* fedHeader = (fedh_t*)payload;
      fedHeader->sourceid = this->fedId_ << FED_SOID_SHIFT;
      fedHeader->eventid  = (FED_SLINK_START_MARKER << FED_HCTRLID_SHIFT) | eventNumber;
      payload += sizeof(fedh_t);
      dataLength += sizeof(fedh_t);
    }

    const uint32_t length = std::min(dataSize-dataOffset,ferolPayloadSize-dataLength);
    memcpy(payload,dataPos+dataOffset,length);
    dataOffset += length;
    payload += length;
    dataLength += length;

    if (packetNumber == ferolBlocks-1)
    {
      ferolHeader->set_last_packet();
      fedt_t* fedTrailer = (fedt_t*)payload;
      payload += sizeof(fedt_t);
      dataLength += sizeof(fedt_t);

      fedTrailer->eventsize = (FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT) | (fedSize>>3);
      fedTrailer->conscheck &= ~(FED_CRCS_MASK | 0xC004);
      crcCalculator_.compute(crc,payload-dataLength,dataLength);
      fedTrailer->conscheck |= (crc << FED_CRCS_SHIFT);
    }
    else
    {
      crcCalculator_.compute(crc,payload-dataLength,dataLength);
    }

    assert( dataLength <= ferolPayloadSize );
    ferolHeader->set_data_length(dataLength);
  }

  assert( dataOffset == dataSize );

  fedFragment = this->fedFragmentFactory_.getFedFragment(this->fedId_,this->isMasterStream_,evbId,bufRef);
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
