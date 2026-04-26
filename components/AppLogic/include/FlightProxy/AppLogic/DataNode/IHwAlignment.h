#pragma once
#include "FlightProxy/Core/FlightProxyTypes.h"

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {

class IHwAlignment {
public:
  virtual ~IHwAlignment() = default;

  virtual Core::AttitudeData
  alignAttitude(const Core::AttitudeData &raw_attitude) const = 0;

  virtual Core::IMUData alignIMU(const Core::IMUData &raw_imu) const = 0;

  virtual Core::MagData alignMag(const Core::MagData &raw_mag) const = 0;

  virtual Core::GPSData alignGPS(const Core::GPSData &raw_gps) const = 0;

  virtual Core::BaroData alignBaro(const Core::BaroData &raw_baro) const = 0;
};

} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
