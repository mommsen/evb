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
  s << "instLumi:         " << luminosity.instLumi << std::endl;
  s << "avgPileUp:        " << luminosity.avgPileUp << std::endl;

  return s;
}


MetaData::BeamSpot::BeamSpot() :
  timeStamp(0),
  x(0),
  y(0),
  z(0),
  dxdz(0),
  dydz(0),
  err_x(0),
  err_y(0),
  err_z(0),
  err_dxdz(0),
  err_dydz(0),
  width_x(0),
  width_y(0),
  sigma_z(0),
  err_width_x(0),
  err_width_y(0),
  err_sigma_z(0)
{};


bool MetaData::BeamSpot::operator!=(const MetaData::BeamSpot& other) const
{
  return (timeStamp != other.timeStamp ||
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


std::ostream& operator<<(std::ostream& s, const MetaData::BeamSpot& beamSpot)
{
  time_t ts = beamSpot.timeStamp / 1000.;

  s << "timeStamp:        " << asctime(localtime(&ts)) << std::endl;
  s << "x:                " << beamSpot.x << std::endl;
  s << "y:                " << beamSpot.y << std::endl;
  s << "z:                " << beamSpot.z << std::endl;
  s << "dxdz:             " << beamSpot.dxdz << std::endl;
  s << "dydz:	          " << beamSpot.dydz << std::endl;
  s << "err_x:            " << beamSpot.err_x << std::endl;
  s << "err_y:            " << beamSpot.err_y << std::endl;
  s << "err_z:            " << beamSpot.err_z << std::endl;
  s << "err_dxdz:         " << beamSpot.err_dxdz << std::endl;
  s << "err_dydz:         " << beamSpot.err_dydz << std::endl;
  s << "width_x:          " << beamSpot.width_x << std::endl;
  s << "width_y:          " << beamSpot.width_y << std::endl;
  s << "sigma_z:          " << beamSpot.sigma_z << std::endl;
  s << "err_width_x:      " << beamSpot.err_width_x << std::endl;
  s << "err_width_y:      " << beamSpot.err_width_y << std::endl;
  s << "err_sigma_z:      " << beamSpot.err_sigma_z << std::endl;

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
  s << "magnetCurrent:    " << std::fixed << std::setprecision(3) << dcs.magnetCurrent << " A" << std::endl;

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
