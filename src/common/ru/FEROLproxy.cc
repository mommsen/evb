#include <algorithm>
#include <byteswap.h>
#include <iterator>
#include <string.h>

#include "interface/shared/fed_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "evb/RU.h"
#include "evb/ru/InputHandler.h"
#include "evb/Constants.h"
#include "evb/Exception.h"
#include "toolbox/net/URN.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"


evb::ru::FEROLproxy::FEROLproxy
(
  boost::shared_ptr<RU> ru,
  SuperFragmentTablePtr sft
) :
InputHandler(ru,sft)
{
}


void evb::ru::FEROLproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  updateInputCounters(bufRef);
  superFragmentTable_->addFragment(bufRef);
}


void evb::ru::FEROLproxy::updateInputCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(inputMonitoringMutex_);

  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const uint32_t payload =
    (stdMsg->MessageSize << 2) - sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  
  char* i2oPayloadPtr = (char*)bufRef->getDataLocation() +
    sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint64_t h0 = bswap_64(*((uint64_t*)(i2oPayloadPtr)));
  const uint64_t eventNumber = (h0 & 0x0000000000FFFFFF);
  
  inputMonitoring_.lastEventNumber = eventNumber;
  inputMonitoring_.payload += payload;
  ++inputMonitoring_.i2oCount;
  ++inputMonitoring_.logicalCount;
}


void evb::ru::FEROLproxy::configure(const Configuration& conf)
{
  clear();
}


void evb::ru::FEROLproxy::clear()
{}


void evb::ru::FEROLproxy::printHtml(xgi::Output *out)
{
  boost::mutex::scoped_lock sl(inputMonitoringMutex_);
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last evt number from FEROL</td>"                   << std::endl;
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


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
