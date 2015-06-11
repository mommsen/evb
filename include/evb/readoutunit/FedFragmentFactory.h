#ifndef _evb_readoutunit_FedFragmentFactory_h_
#define _evb_readoutunit_FedFragmentFactory_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/readoutunit/SocketBuffer.h"
#include "evb/readoutunit/FedFragment.h"
#include "evb/readoutunit/StateMachine.h"
#include "tcpla/MemoryCache.h"
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

      FedFragmentFactory(ReadoutUnit*);

      FedFragmentPtr getFedFragment();
      FedFragmentPtr getFedFragment(toolbox::mem::Reference*, tcpla::MemoryCache*);
      FedFragmentPtr getFedFragment(uint16_t fedId, const EvBid&, toolbox::mem::Reference*);

      void append(FedFragmentPtr&, SocketBufferPtr&, uint32_t& usedSize);

      void reset(const uint32_t runNumber);
      void writeFragmentToFile(const FedFragmentPtr&,const std::string& reasonFordump) const;

      uint32_t getCorruptedEvents() const { return fedErrors_.corruptedEvents; }
      uint32_t getCRCerrors() const { return fedErrors_.crcErrors; }


    private:

      void errorHandler(const FedFragmentPtr&);

      ReadoutUnit* readoutUnit_;
      uint32_t runNumber_;

      CRCCalculator crcCalculator_;

      struct FedErrors
      {
        uint32_t corruptedEvents;
        uint32_t crcErrors;
        uint32_t fedErrors;
        uint32_t nbDumps;

        FedErrors() { reset(); }
        void reset()
        { corruptedEvents=0;crcErrors=0;fedErrors=0;nbDumps=0; }
      } fedErrors_;

    };

  } } //namespace evb::readoutunit


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////


template<class ReadoutUnit>
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::FedFragmentFactory(ReadoutUnit* readoutUnit) :
  readoutUnit_(readoutUnit),
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
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::getFedFragment()
{
  return FedFragmentPtr( new FedFragment() );
}


template<class ReadoutUnit>
evb::readoutunit::FedFragmentPtr
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::getFedFragment(toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
{
  FedFragmentPtr fedFragment( new FedFragment() );
  try
  {
    fedFragment->append(bufRef,cache);
  }
  catch(...)
  {
    errorHandler(fedFragment);
  }

  return fedFragment;
}


template<class ReadoutUnit>
evb::readoutunit::FedFragmentPtr
evb::readoutunit::FedFragmentFactory<ReadoutUnit>::getFedFragment(uint16_t fedId, const EvBid& evbId, toolbox::mem::Reference* bufRef)
{
  FedFragmentPtr fedFragment( new FedFragment() );
  try
  {
    fedFragment->append(fedId,evbId,bufRef);
  }
  catch(...)
  {
    errorHandler(fedFragment);
  }

  return fedFragment;
}


template<class ReadoutUnit>
void evb::readoutunit::FedFragmentFactory<ReadoutUnit>::append(FedFragmentPtr& fedFragment, SocketBufferPtr& socketBuffer, uint32_t& usedSize)
{
  try
  {
    fedFragment->append(socketBuffer,usedSize);
  }
  catch(...)
  {
    errorHandler(fedFragment);
  }
}


template<class ReadoutUnit>
void evb::readoutunit::FedFragmentFactory<ReadoutUnit>::errorHandler(const FedFragmentPtr& fedFragment)
{
  try
  {
    throw;
  }
  catch(exception::DataCorruption& e)
  {
    ++fedErrors_.corruptedEvents;

    if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());

    if ( readoutUnit_->getConfiguration()->tolerateCorruptedEvents )
    {
      LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                      xcept::stdformat_exception_history(e));
      readoutUnit_->notifyQualified("error",e);
    }
    else
    {
      readoutUnit_->getStateMachine()->processFSMEvent( Fail(e) );
      return;
    }
  }
  catch(exception::FEDerror& e)
  {
    ++fedErrors_.fedErrors;

    LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("error",e);

    if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());
  }
  catch(exception::CRCerror& e)
  {
    ++fedErrors_.crcErrors;

    LOG4CPLUS_ERROR(readoutUnit_->getApplicationLogger(),
                    xcept::stdformat_exception_history(e));
    readoutUnit_->notifyQualified("error",e);

    if ( ++fedErrors_.nbDumps <= readoutUnit_->getConfiguration()->maxDumpsPerFED )
      writeFragmentToFile(fedFragment,e.message());
  }
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
