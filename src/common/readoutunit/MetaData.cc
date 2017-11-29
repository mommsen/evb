#include <iomanip>
#include <time.h>

#include "evb/readoutunit/MetaData.h"


MetaData::Luminosity::Luminosity() :
  timeStamp(0),
  lumiSection(0),
  lumiNibble(0),
  instLumi(-1),
  avgPileUp(-1)
{};


bool MetaData::Luminosity::operator!=(const MetaData::Luminosity& other) const
{
  return (timeStamp != other.timeStamp ||
          lumiSection != other.lumiSection ||
          lumiNibble != other.lumiNibble ||
          instLumi != other.instLumi ||
          avgPileUp != other.avgPileUp);
}


std::ostream& operator<<(std::ostream& s, const MetaData::Luminosity& luminosity)
{
  time_t ts = luminosity.timeStamp / 1000.;

  s << "timeStamp:        " << asctime(localtime(&ts)) << std::endl;
  s << "lumiSection:      " << luminosity.lumiSection << std::endl;
  s << "lumiNibble:       " << luminosity.lumiNibble << std::endl;

  std::streamsize ss = s.precision();
  s.setf(std::ios::fixed);
  s.precision(2);
  s << "instLumi:         " << luminosity.instLumi << std::endl;
  s << "avgPileUp:        " << luminosity.avgPileUp << std::endl;
  s.unsetf(std::ios::fixed);
  s.precision(ss);

  return s;
}


MetaData::BeamSpot::BeamSpot() :
  timeStamp(0),
  x(0),
  y(0),
  z(0),
  dxdz(0),
  dydz(0),
  errX(0),
  errY(0),
  errZ(0),
  errDxdz(0),
  errDydz(0),
  widthX(0),
  widthY(0),
  sigmaZ(0),
  errWidthX(0),
  errWidthY(0),
  errSigmaZ(0)
{};


bool MetaData::BeamSpot::operator!=(const MetaData::BeamSpot& other) const
{
  return (timeStamp != other.timeStamp ||
          x != other.x ||
          y != other.y ||
          z != other.z ||
          dxdz != other.dxdz ||
          dydz != other.dydz ||
          errX != other.errX ||
          errY != other.errY ||
          errZ != other.errZ ||
          errDxdz != other.errDxdz ||
          errDydz != other.errDydz ||
          widthX != other.widthX ||
          widthY != other.widthY ||
          sigmaZ != other.sigmaZ ||
          errWidthX != other.errWidthX ||
          errWidthY != other.errWidthY ||
          errSigmaZ != other.errSigmaZ);
}


std::ostream& operator<<(std::ostream& s, const MetaData::BeamSpot& beamSpot)
{
  time_t ts = beamSpot.timeStamp / 1000.;

  s << "timeStamp:         " << asctime(localtime(&ts)) << std::endl;

  std::streamsize ss = s.precision();
  s.setf(std::ios::fixed);
  s.precision(6);
  s << "x:                 " << beamSpot.x << std::endl;
  s << "y:                 " << beamSpot.y << std::endl;
  s << "z:                 " << beamSpot.z << std::endl;
  s << "dxdz:              " << beamSpot.dxdz << std::endl;
  s << "dydz:	           " << beamSpot.dydz << std::endl;
  s << "err of x:          " << beamSpot.errX << std::endl;
  s << "err of y:          " << beamSpot.errX << std::endl;
  s << "err of z:          " << beamSpot.errZ << std::endl;
  s << "err of dxdz:       " << beamSpot.errDxdz << std::endl;
  s << "err of dydz:       " << beamSpot.errDydz << std::endl;
  s << "width in x:        " << beamSpot.widthX << std::endl;
  s << "width in y:        " << beamSpot.widthY << std::endl;
  s << "sigma z:           " << beamSpot.sigmaZ << std::endl;
  s << "err of width in x: " << beamSpot.errWidthX << std::endl;
  s << "err of width in y  " << beamSpot.errWidthY << std::endl;
  s << "err of sigma z:    " << beamSpot.errSigmaZ << std::endl;
  s.unsetf(std::ios::fixed);
  s.precision(ss);

  return s;
}


MetaData::DCS::DCS() :
  timeStamp(0),
  highVoltageReady(0),
  magnetCurrent(-1)
{};


bool MetaData::DCS::operator!=(const MetaData::DCS& other) const
{
  return (timeStamp != other.timeStamp ||
          highVoltageReady != other.highVoltageReady ||
          magnetCurrent != other.magnetCurrent);
}


std::ostream& operator<<(std::ostream& s, const MetaData::DCS& dcs)
{
  time_t ts = dcs.timeStamp / 1000.;

  s << "timeStamp:        " << asctime(localtime(&ts)) << std::endl;
  s << "highVoltageReady: " << dcs.highVoltageReady << std::endl;

  std::streamsize ss = s.precision();
  s.setf(std::ios::fixed);
  s.precision(3);
  s << "magnetCurrent:    " << dcs.magnetCurrent << " A" << std::endl;
  s.unsetf(std::ios::fixed);
  s.precision(ss);

  return s;
}


MetaData::Data::Data() :
  version(MetaData::version)
{};


bool MetaData::Data::operator!=(const MetaData::Data& other)
{
  return (luminosity != other.luminosity ||
          beamSpot != other.beamSpot ||
          dcs != dcs);
}


std::ostream& operator<<(std::ostream& s, const MetaData::Data& data)
{
  s << "version:          " << data.version << std::endl;
  s << data.luminosity << std::endl;
  s << data.beamSpot << std::endl;
  s << data.dcs << std::endl;

  return s;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
