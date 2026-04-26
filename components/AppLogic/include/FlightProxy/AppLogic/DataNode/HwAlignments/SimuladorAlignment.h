#include "FlightProxy/AppLogic/DataNode/IHwAlignment.h"

#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
namespace HwAlignment {
class SimuladorAlignment
    : public IHwAlignment,
      public std::enable_shared_from_this<SimuladorAlignment> {
public:
  Core::AttitudeData
  alignAttitude(const Core::AttitudeData &raw_attitude) const override {
    Core::AttitudeData alignedAttitude = raw_attitude;
    alignedAttitude.roll = raw_attitude.roll * (std::numbers::pi / 180);
    alignedAttitude.pitch = -raw_attitude.pitch * (std::numbers::pi / 180);
    alignedAttitude.yaw = raw_attitude.yaw * (std::numbers::pi / 180);
    return alignedAttitude;
  }

  Core::IMUData alignIMU(const Core::IMUData &raw_imu) const override {
    Core::IMUData alignedIMU = raw_imu;
    alignedIMU.gyro_x = raw_imu.gyro_x * (std::numbers::pi / 180);
    alignedIMU.gyro_y = -raw_imu.gyro_y * (std::numbers::pi / 180);
    alignedIMU.gyro_z = -raw_imu.gyro_z * (std::numbers::pi / 180);
    return alignedIMU;
  }

  Core::MagData alignMag(const Core::MagData &raw_mag) const override {
    Core::MagData alignedMag = raw_mag;
    alignedMag.mag_x = -raw_mag.mag_y;
    alignedMag.mag_y = -raw_mag.mag_x;
    return alignedMag;
  }

  Core::GPSData alignGPS(const Core::GPSData &raw_gps) const override {
    Core::GPSData alignedGPS = raw_gps;
    alignedGPS.speed = raw_gps.speed * 0.514444;
    alignedGPS.heading = raw_gps.heading * (std::numbers::pi / 180);
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
