#include "evb/ru/InputHandler.h"
#include "evb/Exception.h"


evb::ru::FBOproxy::FBOproxy(xdaq::Application* app) :
InputHandler(app),
blockFIFO_("blockFIFO"),
dropInputData_(false),
superFragmentHead_(0),
superFragmentTail_(0)
{}


void evb::ru::FBOproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  toolbox::mem::Reference *nextBufRef = 0;

  // Break the chain (if there is one) into separate blocks
  while (bufRef)
  {
    nextBufRef = bufRef->getNextReference();
    bufRef->setNextReference(0);
    
    const I2O_MESSAGE_FRAME* stdMsg =
      (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();

    updateInputCounters(stdMsg);

    // Tell the reference of the block's buffer the size of the data in the buffer.
    bufRef->setDataSize(stdMsg->MessageSize << 2);

    appendBlockToSuperFragment(bufRef);

    handleCompleteSuperFragment();

    bufRef = nextBufRef;
  }
}


void evb::ru::FBOproxy::updateInputCounters(const I2O_MESSAGE_FRAME* stdMsg)
{
  boost::mutex::scoped_lock sl(inputMonitoringMutex_);

  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const uint32_t payload =
    (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

  inputMonitoring_.lastEventNumber = block->eventNumber;
  inputMonitoring_.payload += payload;
  ++inputMonitoring_.i2oCount;
  if (block->blockNb == (block->nbBlocksInSuperFragment - 1))
  {
    ++inputMonitoring_.logicalCount;
  }
}


void evb::ru::FBOproxy::appendBlockToSuperFragment
(
  toolbox::mem::Reference* bufRef
)
{
  if (superFragmentHead_)
  {
    superFragmentTail_->setNextReference(bufRef);
    superFragmentTail_ = bufRef;
  }
  else
  { 
    superFragmentHead_ = bufRef;
    superFragmentTail_ = bufRef;
  }
}


void evb::ru::FBOproxy::handleCompleteSuperFragment()
{
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)superFragmentTail_->getDataLocation();

  if ( block->blockNb == (block->nbBlocksInSuperFragment-1) )
  {
    if ( dropInputData_ )
    {
      superFragmentHead_->release();
    }
    else
    {
      while ( ! blockFIFO_.enq(superFragmentHead_) ) ::usleep(1000);
    }

    superFragmentHead_ = 0;
    superFragmentTail_ = 0;
  }
}


bool evb::ru::FBOproxy::getData
(
  const EvBid& evbId,
  toolbox::mem::Reference*& bufRef
)
{
  if ( ! blockFIFO_.deq(bufRef) ) return false;

  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  
  const EvBid blockEvBid = evbIdFactory_.getEvBid(block->eventNumber);
  if ( blockEvBid != evbId )
  {
    std::stringstream oss;
    
    oss << "Mismatch detected: expected evb id "
      << evbId << ", but found evb id "
      << blockEvBid << " in data block.";

    XCEPT_RAISE(exception::MismatchDetected, oss.str());
  }

  block->resyncCount = blockEvBid.resyncCount();

  return true;
}


void evb::ru::FBOproxy::configure(const Configuration& conf)
{
  clear();

  blockFIFO_.resize(conf.blockFIFOCapacity);
  dropInputData_ = conf.dropInputData;
}


void evb::ru::FBOproxy::clear()
{
  if (superFragmentHead_)
  {
    superFragmentHead_->release();
    
    superFragmentHead_     = 0;
    superFragmentTail_     = 0;
  }

  toolbox::mem::Reference* bufRef;
  while ( blockFIFO_.deq(bufRef) ) { bufRef->release(); }

  evbIdFactory_.reset();
}


void evb::ru::FBOproxy::printHtml(xgi::Output *out)
{
  {
    boost::mutex::scoped_lock sl(inputMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from FBO</td>"                     << std::endl;
    *out << "<td>" << inputMonitoring_.lastEventNumber << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">RU input</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << inputMonitoring_.payload / 0x100000<< "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << inputMonitoring_.logicalCount << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << inputMonitoring_.i2oCount << "</td>"          << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  blockFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
