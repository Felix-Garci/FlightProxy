#include "FlightProxy/AppLogic/DataNode/IHwAlignment.h"

#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
namespace HwAlignment {
class SimuladorAlignment
    : public IHwAlignment,
      public std::enable_shared_from_this<SimuladorAlignment> {

  Core::AttitudeData
  alignAttitude(const Core::AttitudeData &raw_attitude) const override {
    Core::AttitudeData alignedAttitude = raw_attitude;
    return alignedAttitude;
  }

  Core::IMUData alignIMU(const Core::IMUData &raw_imu) const override {
    Core::IMUData alignedIMU = raw_imu;
    return alignedIMU;
  }

  Core::MagData alignMag(const Core::MagData &raw_mag) const override {
    Core::MagData alignedMag = raw_mag;
    return alignedMag;
  }

  Core::GPSData alignGPS(const Core::GPSData &raw_gps) const override {
    Core::GPSData alignedGPS = raw_gps;
    return alignedGPS;
  }

  Core::BaroData alignBaro(const Core::BaroData &raw_baro) const override {
    Core::BaroData alignedBaro = raw_baro;
    return alignedBaro;
  }
};
} // namespace HwAlignment
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
