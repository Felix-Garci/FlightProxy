#pragma once
#include "Controls/position_vertical.h"
#include "Controls/velocity_vertical.h"

#include "Controls/velocity_frontal.h"

#include "Controls/velocity_lateral.h"

#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class ControlUniversal {
public:
  ControlUniversal();
  void init(std::shared_ptr<AlmacenFlexible> bb);
  Core::RCNORMData step(uint8_t ctrlLevel, Core::RCNORMData rcNormData,
                        float dt);

private:
  // Data getters
  std::function<uint8_t(void)> levelGetter_;
  uint8_t levelData_;
  uint8_t prevLevel_;

  std::function<Core::BaroData(void)> baroGetter_;
  Core::BaroData baroData_;

  std::function<Core::IMUData(void)> imuGetter_;
  Core::IMUData imuData_;

  std::function<Core::GPSData(void)> gpsGetter_;
  Core::GPSData gpsData_;

  std::function<Core::AttitudeData(void)> attitudeGetter_;
  Core::AttitudeData attitudeData_;

  std::function<Core::MagData(void)> magGetter_;
  Core::MagData magData_;

  std::function<void(Core::VelocityData)> velSetter_;
  Core::VelocityData velData_;

  float yawRealRad_ = 0;

  // COntroles
  Controls::velocity_vertical velocity_vertical_ =
      Controls::velocity_vertical();

  Controls::position_vertical position_vertical_ =
      Controls::position_vertical();

  Controls::velocity_frontal velocity_frontal_ = Controls::velocity_frontal();

  Controls::velocity_lateral velocity_lateral_ = Controls::velocity_lateral();

  // Math funcions
  void magCompensation();
  void eulerProyection(float dt);
  void vGps2DroneRef(float &vx, float &vy);
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
