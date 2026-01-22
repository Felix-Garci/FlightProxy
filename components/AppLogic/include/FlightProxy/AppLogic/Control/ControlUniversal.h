#pragma once
#include "Controls/velocity_vertical.h"
#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class ControlUniversal {
public:
  ControlUniversal();
  void init(std::shared_ptr<AlmacenFlexible> bb);
  Core::RCNORMData step(uint8_t ctrlLevel, Core::RCNORMData rcNormData);

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

  // COntroles
  Controls::velocity_vertical velocity_vertical_ =
      Controls::velocity_vertical();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
