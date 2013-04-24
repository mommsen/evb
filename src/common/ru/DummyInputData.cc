#include "evb/RU.h"
#include "evb/ru/InputHandler.h"
#include "evb/Exception.h"
#include "toolbox/mem/exception/Exception.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/net/URN.h"
#include "xcept/tools.h"

#include <iomanip>


evb::ru::DummyInputData::DummyInputData
(
  boost::shared_ptr<RU> ru,
  SuperFragmentTablePtr sft
) :
InputHandler(ru,sft),
superFragmentGenerator_(ru->getApplicationDescriptor()->getURN())
{}


void evb::ru::DummyInputData::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  XCEPT_RAISE(exception::Configuration,
    "Received an event fragment while generating dummy data");
}


// bool evb::ru::DummyInputData::getData
// (
//   const EvBid& evbId,
//   toolbox::mem::Reference*& bufRef
// )
// {
//   if ( superFragmentGenerator_.getData(bufRef,evbId) )
//   {
//     const I2O_MESSAGE_FRAME* stdMsg =
//       (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
//     const uint32_t payload =
//       (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

//     inputMonitoring_.lastEventNumber = evbId.eventNumber();
//     inputMonitoring_.payload += payload;
//     ++inputMonitoring_.logicalCount;
//     return true;
//   }
//   return false;
// }


void evb::ru::DummyInputData::configure(const Configuration& conf)
{
  // superFragmentGenerator_.configure(
  //   conf.fedSourceIds, conf.usePlayback, conf.playbackDataFile,
  //   conf.dummyBlockSize, conf.dummyFedPayloadSize, conf.dummyFedPayloadStdDev);
}


void evb::ru::DummyInputData::printHtml(xgi::Output* out)
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << "Last evt number generated"                             << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << inputMonitoring_.lastEventNumber                        << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << "Generated empty super-fragments"                       << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << inputMonitoring_.logicalCount                           << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << "Memory pool usage (kB)"                                << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "<td>"                                                  << std::endl;
  *out << superFragmentGenerator_.getMemoryUsage()/1024           << std::endl;
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
