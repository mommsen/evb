#ifndef _evb_readoutunit_FedFragmentFactory_h_
#define _evb_readoutunit_FedFragmentFactory_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/readoutunit/SocketBuffer.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/StateMachine.h"
#include "toolbox/mem/Reference.h"


namespace evb {

  namespace readoutunit { // namespace evb::readoutunit

    /**
     * \ingroup xdaqApps
     * \brief Factory of FEDFragments
     */

    template<class ReadoutUnit>
    class FedFragmentFactory
    {

    public:

      FedFragmentFactory(ReadoutUnit*, const EvBidFactoryPtr&);

      FedFragmentPtr getFedFragment(const uint16_t fedId, const bool isMasterFed);
      FedFragmentPtr getFedFragment(const uint16_t fedId, const bool isMasterFed, const EvBid&, toolbox::mem::Reference*);

      bool append(FedFragmentPtr&, SocketBufferPtr&, uint32_t& usedSize);

      void reset(const uint32_t runNumber);
      void writeFragmentToFile(const FedFragmentPtr&,const std::string& reasonFordump) const;

      uint32_t getCorruptedEvents() const { return fedErrors_.corruptedEvents; }
      uint32_t getEventsOutOfSequence() const { return fedErrors_.eventsOutOfSequence; }
      uint32_t getCRCerrors() const { return fedErrors_.crcErrors; }


    private:

      FedFragmentPtr makeFedFragment(const uint16_t fedId, const bool isMasterFed);
      bool errorHandler(const FedFragmentPtr&);

      ReadoutUnit* readoutUnit_;
      const EvBidFactoryPtr& evbIdFactory_;
      uint32_t runNumber_;

      static CRCCalculator crcCalculator_;

      struct FedErrors
      {
        uint32_t corruptedEvents;
        uint32_t eventsOutOfSequence;
        uint32_t crcErrors;
        uint32_t fedErrors;
        uint32_t nbDumps;

        FedErrors() { reset(); }
        void reset()
        { corruptedEvents=0;eventsOutOfSequence=0;crcErrors=0;fedErrors=0;nbDumps=0; }
      } fedErrors_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class ReadoutUnit>
evb::CRCCalculator evb::readoutunit::FedFragmentFactory<ReadoutUnit>::crcCalculator_;


template<class ReadoutUnit>
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::FedFragmentFactory(ReadoutUnit* readoutUnit, const EvBidFactoryPtr& evbIdFactory) :
  readoutUnit_(readoutUnit),
  evbIdFactory_(evbIdFactory),
  runNumber_(0)
{}


template<class ReadoutUnit>
void evb::readoutunit::FedFragmentFactory<ReadoutUnit>::reset(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  fedErrors_.reset();
}


template<class ReadoutUnit>
evb::readoutunit::FedFragmentPtr
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::getFedFragment
(
  const uint16_t fedId,
  const bool isMasterFed
)
{
  return makeFedFragment(fedId,isMasterFed);
}


template<class ReadoutUnit>
evb::readoutunit::FedFragmentPtr
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::getFedFragment
(
  const uint16_t fedId,
  const bool isMasterFed,
  const EvBid& evbId,
  toolbox::mem::Reference* bufRef
)
{
  const FedFragmentPtr fedFragment = makeFedFragment(fedId,isMasterFed);
  try
  {
    fedFragment->append(evbId,bufRef);
  }
  catch(...)
  {
    errorHandler(fedFragment);
  }

  return fedFragment;
}


template<class ReadoutUnit>
bool evb::readoutunit::FedFragmentFactory<ReadoutUnit>::append
(
  FedFragmentPtr& fedFragment,
  SocketBufferPtr& socketBuffer,
  uint32_t& usedSize
)
{
  try
  {
    return fedFragment->append(socketBuffer,usedSize);
  }
  catch(...)
  {
    return errorHandler(fedFragment);
  }
  return false;
}


template<class ReadoutUnit>
evb::readoutunit::FedFragmentPtr evb::readoutunit::FedFragmentFactory<ReadoutUnit>::makeFedFragment
(
  const uint16_t fedId,
  const bool isMasterFed
)
{
  return FedFragmentPtr(
    new FedFragment(fedId,
                    isMasterFed,
                    readoutUnit_->getSubSystem(),
                    evbIdFactory_,
                    readoutUnit_->getConfiguration()->checkCRC,
                    fedErrors_.fedErrors,
                    fedErrors_.crcErrors
    )
  );
}


template<class ReadoutUnit>
bool evb::readoutunit::FedFragmentFactory<ReadoutUnit>::errorHandler(const FedFragmentPtr& fedFragment)
{
  try
  {
    throw;
  }
  catch(exception::FEDerror& e)
  {
    LOG4CPLUS_WARN(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("warn",e);

    if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());
  }
  catch(exception::CRCerror& e)
  {
    LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("error",e);

    if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());
  }
  catch(exception::DataCorruption& e)
  {
    ++fedErrors_.corruptedEvents;

    if ( readoutUnit_->getConfiguration()->tolerateCorruptedEvents
         && ! fedFragment->isMasterFed() )
    {
      if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
        writeFragmentToFile(fedFragment,e.message());

      readoutUnit_->getStateMachine()->processFSMEvent( DataLoss(e,fedFragment->getFedId()) );
    }
    else
    {
      writeFragmentToFile(fedFragment,e.message());
      readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
    }
  }
  catch(exception::EventOutOfSequence& e)
  {
    ++fedErrors_.eventsOutOfSequence;

    if ( readoutUnit_->getConfiguration()->tolerateOutOfSequenceEvents
         && ! fedFragment->isMasterFed() )
    {
      if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
        writeFragmentToFile(fedFragment,e.message());

      readoutUnit_->getStateMachine()->processFSMEvent( DataLoss(e,fedFragment->getFedId()) );
    }
    else
    {
      writeFragmentToFile(fedFragment,e.message());
      readoutUnit_->getStateMachine()->processFSMEvent( EventOutOfSequence(e) );
    }
  }
  catch(exception::TCDS& e)
  {
    std::ostringstream msg;
    msg << "Failed to extract lumi section from FED " << fedFragment->getFedId();

    XCEPT_DECLARE_NESTED(exception::TCDS, sentinelException,
                         msg.str(),e);
    msg << ": " << e.message();
    writeFragmentToFile(fedFragment,msg.str());

    readoutUnit_->getStateMachine()->processFSMEvent( Fail(sentinelException) );
  }

  return true;
}


template<class ReadoutUnit>
void evb::readoutunit::FedFragmentFactory<ReadoutUnit>::writeFragmentToFile
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
  fragment->dump(dumpFile,reasonForDump);
  dumpFile.close();
}


#endif // _evb_readoutunit_FedFragmentFactory_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
