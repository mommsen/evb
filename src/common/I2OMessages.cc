#include "evb/I2OMessages.h"


uint32_t evb::msg::ReadoutMsg::getHeaderSize() const
{
  return
    sizeof(msg::ReadoutMsg) +
    nbRequests * sizeof(EvBid) +
    ((nbRUtids+1)&~1) * sizeof(I2O_TID); // there's always an even number of I2O_TIDs to keep 64-bit alignment
}


void evb::msg::ReadoutMsg::getEvBids(evb::msg::EvBids& ids) const
{
  ids.clear();
  ids.reserve(nbRequests);

  unsigned char* payload = (unsigned char*)&evbIds[0];

  for (uint32_t i=0; i < nbRequests; ++i)
  {
    ids.push_back( *(EvBid*)payload );
    payload += sizeof(EvBid);
  }
}


void evb::msg::ReadoutMsg::getRUtids(evb::msg::RUtids& tids) const
{
  tids.clear();
  tids.reserve(nbRUtids);

  unsigned char* payload = (unsigned char*)&evbIds[0] +
    nbRequests * sizeof(EvBid);

  for (uint32_t i=0; i < nbRUtids; ++i)
  {
    tids.push_back( *(I2O_TID*)payload );
    payload += sizeof(I2O_TID);
  }
}


uint32_t evb::msg::SuperFragment::getHeaderSize() const
{
  return
    sizeof(msg::SuperFragment) +
    ((nbDroppedFeds-1 + 3) / 4) * sizeof(uint64_t);
  // one FED id is accounted in sizeof(msg::SuperFragment)
  // after that groups of 4 are needed to keep 64-bit alignment
}


void evb::msg::SuperFragment::appendFedIds(evb::msg::FedIds& feds) const
{
  unsigned char* payload = (unsigned char*)&fedIds[0];

  for (uint16_t i=0; i < nbDroppedFeds; ++i)
  {
    feds.push_back( *(uint16_t*)payload );
    payload += sizeof(uint16_t);
  }
}



uint32_t evb::msg::I2O_DATA_BLOCK_MESSAGE_FRAME::getHeaderSize() const
{
  return
    sizeof(msg::I2O_DATA_BLOCK_MESSAGE_FRAME) +
    nbSuperFragments * sizeof(EvBid) +
    ((nbRUtids+1)&~1) * sizeof(I2O_TID); // there's always an even number of I2O_TIDs to keep 64-bit alignment
}


void evb::msg::I2O_DATA_BLOCK_MESSAGE_FRAME::getEvBids(evb::msg::EvBids& ids) const
{
  ids.clear();
  ids.reserve(nbSuperFragments);

  unsigned char* payload = (unsigned char*)&evbIds[0];

  for (uint32_t i=0; i < nbSuperFragments; ++i)
  {
    ids.push_back( *(EvBid*)payload );
    payload += sizeof(EvBid);
  }
}


void evb::msg::I2O_DATA_BLOCK_MESSAGE_FRAME::getRUtids(evb::msg::RUtids& tids) const
{
  tids.clear();
  tids.reserve(nbRUtids);

  unsigned char* payload = (unsigned char*)&evbIds[0] +
    nbSuperFragments * sizeof(EvBid);

  for (uint32_t i=0; i < nbRUtids; ++i)
  {
    tids.push_back( *(I2O_TID*)payload );
    payload += sizeof(I2O_TID);
  }
}


std::ostream& evb::msg::operator<<
(
  std::ostream& str,
  const I2O_PRIVATE_MESSAGE_FRAME pvtMessageFrame
)
{
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


std::ostream& evb::msg::operator<<
(
  std::ostream& str,
  const evb::msg::ReadoutMsg readoutMsg
)
{
  str << "ReadoutMsg:" << std::endl;

  str << readoutMsg.PvtMessageFrame;

  str << "buTid=" << readoutMsg.buTid << std::endl;
  str << "buResourceId=" << readoutMsg.buResourceId << std::endl;
  str << "nbRequests=" << readoutMsg.nbRequests << std::endl;
  str << "nbRUtids=" << readoutMsg.nbRUtids << std::endl;

  return str;
}


std::ostream& evb::msg::operator<<
(
  std::ostream& str,
  const evb::msg::SuperFragment superFragment
)
{
  str << "SuperFragment:" << std::endl;

  str << "superFragmentNb=" << superFragment.superFragmentNb << std::endl;
  str << "totalSize=" << superFragment.totalSize << std::endl;
  str << "partSize=" << superFragment.partSize << std::endl;
  str << "nbDroppedFeds=" << superFragment.nbDroppedFeds << std::endl;

  if ( superFragment.nbDroppedFeds > 0 )
  {
    evb::msg::FedIds fedIds;
    fedIds.reserve(superFragment.nbDroppedFeds);
    superFragment.appendFedIds(fedIds);
    str << "fedIds:" << std::endl;
    for (uint32_t i=0; i < superFragment.nbDroppedFeds; ++i)
      str << "   [" << i << "]: " << fedIds[i] << std::endl;
  }

  return str;
}


std::ostream& evb::msg::operator<<
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
  str << "nbRUtids=" << dataBlockMsg.nbRUtids << std::endl;

  evb::msg::EvBids evbIds;
  dataBlockMsg.getEvBids(evbIds);
  str << "evbIds:" << std::endl;
  for (uint32_t i=0; i < dataBlockMsg.nbSuperFragments; ++i)
    str << "   [" << i << "]: " << evbIds[i] << std::endl;

  evb::msg::RUtids ruTids;
  dataBlockMsg.getRUtids(ruTids);
  str << "ruTids:" << std::endl;
  for (uint32_t i=0; i < dataBlockMsg.nbRUtids; ++i)
    str << "   [" << i << "]: " << ruTids[i] << std::endl;

  return str;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
