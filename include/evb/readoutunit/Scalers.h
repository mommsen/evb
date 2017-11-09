#ifndef _evb_readoutunit_Scalers_h_
#define _evb_readoutunit_Scalers_h_

#include <stdint.h>

namespace Scalers
{
  struct Luminosity
  {
    float instLumi;
    float avgPileUp;
    uint16_t lumiSection;
    uint16_t lumiNibble;
    bool valid;

    Luminosity() : valid(false) {};

    bool operator!=(const Luminosity& other)
    {
      return (valid != other.valid ||
              lumiSection != other.lumiSection ||
              lumiNibble != other.lumiNibble ||
              instLumi != other.instLumi ||
              avgPileUp != other.avgPileUp);
    }
  };

  struct BeamSpot
  {
    float x;
    float y;
    float z;
    float dxdz;
    float dydz;
    float err_x;
    float err_y;
    float err_z;
    float err_dxdz;
    float err_dydz;
    float width_x;
    float width_y;
    float sigma_z;
    float err_width_x;
    float err_width_y;
    float err_sigma_z;
    bool valid;

    BeamSpot() : valid(false) {};

    bool operator!=(const BeamSpot& other)
    {
      return (valid != other.valid ||
              x != other.x ||
              y != other.y ||
              z != other.z ||
              dxdz != other.dxdz ||
              dydz != other.dydz ||
              err_x != other.err_x ||
              err_y != other.err_y ||
              err_z != other.err_z ||
              err_dxdz != other.err_dxdz ||
              err_dydz != other.err_dydz ||
              width_x != other.width_x ||
              width_y != other.width_y ||
              sigma_z != other.sigma_z ||
              err_width_x != other.err_width_x ||
              err_width_y != other.err_width_y ||
              err_sigma_z != other.err_sigma_z);
    }
  };

  struct Data
  {
    uint64_t timeStamp;
    Luminosity luminosity;
    BeamSpot beamSpot;
    uint16_t highVoltageReady;
    float magnetCurrent;

    Data() : highVoltageReady(0),magnetCurrent(-1) {};

    bool operator!=(const Data& other)
    {
      return (luminosity != other.luminosity ||
              beamSpot != other.beamSpot ||
              highVoltageReady != other.highVoltageReady ||
              magnetCurrent != other.magnetCurrent);
    }

  };

  const size_t dataSize = sizeof(Data);
}

#endif // _evb_readoutunit_Scalers_h_
