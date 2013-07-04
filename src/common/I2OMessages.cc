#include "evb/I2OMessages.h"


std::ostream& operator<<
(
  std::ostream& str,
  const I2O_PRIVATE_MESSAGE_FRAME pvtMessageFrame
)
{
  str << "PvtMessageFrame.StdMessageFrame.VersionOffset=";
  str << pvtMessageFrame.StdMessageFrame.VersionOffset << std::endl;
  
  str << "PvtMessageFrame.StdMessageFrame.MsgFlags=";
  str << pvtMessageFrame.StdMessageFrame.MsgFlags << std::endl;
  
  str << "PvtMessageFrame.StdMessageFrame.MessageSize=";
  str << pvtMessageFrame.StdMessageFrame.MessageSize << std::endl;
  
  str << "PvtMessageFrame.StdMessageFrame.TargetAddress=";
  str << pvtMessageFrame.StdMessageFrame.TargetAddress << std::endl;
  
  str << "PvtMessageFrame.StdMessageFrame.InitiatorAddress=";
  str << pvtMessageFrame.StdMessageFrame.InitiatorAddress << std::endl;
  
  str << "PvtMessageFrame.StdMessageFrame.Function=";
  str << pvtMessageFrame.StdMessageFrame.Function << std::endl;
  
  str << "PvtMessageFrame.XFunctionCode=";
  str << pvtMessageFrame.XFunctionCode << std::endl;
  
  str << "PvtMessageFrame.OrganizationID=";
  str << pvtMessageFrame.OrganizationID << std::endl;

  return str;
}


std::ostream& operator<<
(
  std::ostream& str,
  const evb::msg::RqstForFragmentsMsg rqstForFragmentsMsg
)
{
  str << "RqstForFragmentsMsg:" << std::endl;
  
  str << rqstForFragmentsMsg.PvtMessageFrame;
  
  str << "buResourceId=" << rqstForFragmentsMsg.buResourceId << std::endl;
  str << "nbRequests=" << rqstForFragmentsMsg.nbRequests << std::endl;
  str << "evbIds:" << std::endl;
  for (int32_t i=0; i < rqstForFragmentsMsg.nbRequests; ++i)
    str << "   [" << i << "]: " << rqstForFragmentsMsg.evbIds[i] << std::endl;
  
  return str;
}


std::ostream& operator<<
(
  std::ostream& str,
  const evb::msg::SuperFragment superFragment
)
{
  str << "SuperFragment:" << std::endl;
  
  str << "superFragmentNb=" << superFragment.superFragmentNb << std::endl;
  str << "totalSize=" << superFragment.totalSize << std::endl;
  str << "partSize=" << superFragment.partSize << std::endl;
  
  return str;
}


std::ostream& operator<<
(
  std::ostream& str,
  const evb::msg::I2O_DATA_BLOCK_MESSAGE_FRAME& dataBlockMsg
)
{
  str << "I2O_DATA_BLOCK_MESSAGE_FRAME:" << std::endl;
  
  str << dataBlockMsg.PvtMessageFrame;
  
  str << "buResourceId=" << dataBlockMsg.buResourceId << std::endl;
  str << "nbBlocks=" << dataBlockMsg.nbBlocks << std::endl;
  str << "blockNb=" << dataBlockMsg.blockNb << std::endl;
  str << "nbSuperFragments=" << dataBlockMsg.nbSuperFragments << std::endl;
  str << "evbIds:" << std::endl;
  for (uint32_t i=0; i < dataBlockMsg.nbSuperFragments; ++i)
    str << "   [" << i << "]: " << dataBlockMsg.evbIds[i] << std::endl;
  
  return str;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
