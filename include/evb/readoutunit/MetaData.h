#ifndef _evb_readoutunit_MetaData_h_
#define _evb_readoutunit_MetaData_h_

#include <sstream>
#include <stdint.h>

namespace MetaData
{
  struct Luminosity
  {
    uint64_t timeStamp;
    uint16_t lumiSection;
    uint16_t lumiNibble;
    float instLumi;
    float avgPileUp;

    Luminosity();

    bool operator!=(const Luminosity& other) const;
  };

  struct BeamSpot
  {
    uint64_t timeStamp;
    float x;
    float y;
    float z;
    float dxdz;
    float dydz;
    float errX;
    float errY;
    float errZ;
    float errDxdz;
    float errDydz;
    float widthX;
    float widthY;
    float sigmaZ;
    float errWidthX;
    float errWidthY;
    float errSigmaZ;

    BeamSpot();

    bool operator!=(const BeamSpot& other) const;
  };

  struct DCS
  {
    uint64_t timeStamp;
    uint32_t highVoltageReady;
    float magnetCurrent;

    DCS();

    bool operator!=(const DCS& other) const;
  };

  struct Data
  {
    uint8_t version;
    Luminosity luminosity;
    BeamSpot beamSpot;
    DCS dcs;

    Data();

    bool operator!=(const Data& other);
  };

  const uint8_t version = 1;
  const size_t dataSize = sizeof(Data);

} //namespace MetaData

std::ostream& operator<<(std::ostream&, const MetaData::Luminosity&);
std::ostream& operator<<(std::ostream&, const MetaData::BeamSpot&);
std::ostream& operator<<(std::ostream&, const MetaData::DCS&);
std::ostream& operator<<(std::ostream&, const MetaData::Data&);

#endif // _evb_readoutunit_MetaData_h_



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
