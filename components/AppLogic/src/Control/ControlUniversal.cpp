#include "FlightProxy/AppLogic/Control/ControlUniversal.h"
#include "FlightProxy/AppLogic/AlmazenFlexibleID.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {

ControlUniversal::ControlUniversal() {}

void ControlUniversal::init(std::shared_ptr<AlmacenFlexible> bb) {
  // Data getters:
  levelGetter_ = bb->registrarConsumidor<uint8_t>(ID_CTRL_LVL);
  baroGetter_ = bb->registrarConsumidor<Core::BaroData>(ID_BARO_Data);
  imuGetter_ = bb->registrarConsumidor<Core::IMUData>(ID_IMU_Data);
  gpsGetter_ = bb->registrarConsumidor<Core::GPSData>(ID_GPS_Data);

  // Controles
  auto vvci =
      bb->registrarConsumidor<Core::PidCtrlVertVelIn>(ID_CTRL_VERTVEL_IN);
  auto vvct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_VERTVEL_OUT);
  velocity_vertical_.init(vvci, vvct);
}
Core::RCNORMData ControlUniversal::step(uint8_t ctrlLevel,
                                        Core::RCNORMData rcNormData) {
  levelData_ = levelGetter_();
  baroData_ = baroGetter_();
  // imuData_ = imuGetter_();
  // gpsData_ = gpsGetter_();

  ctrlLevel = ctrlLevel > levelData_ ? levelData_ : ctrlLevel;

  if (prevLevel_ != ctrlLevel) {
    FP_LOG_D("ControlUniversal", "ctrl lvl from %d => %d", prevLevel_,
             ctrlLevel);

    velocity_vertical_.reset();
    prevLevel_ = ctrlLevel;
  }

  switch (ctrlLevel) {
  case 5: // Path planner

    // x,y,z,grad = pathplaner(newxWP)

  case 4: // Coordinates + orientation

    // roll,pitch=posicion_horizontal(x,y,gps)->(-1,1)(-1,1)/horizontal_abs_vel_ref
    // throttle=posicion_vertical(z,baro)->(-1,1)/vertical_vel_ref
    // yaw=horientazion(abs_grad,brujula)->(-1,1)/angular_vel_ref

  case 3: // Absolute vel + w

    // roll,pitch=reff_transform(roll(-1,1),pitch(-1,1),brujula)->(-1,1)(-1,1)/horizontal_rel_vel

  case 2: // Relative vel + w

    // roll=lateral_vel(roll(-1,1),imu+gps)->roll(-1,1)
    // pitch=frontal_vel(pitch(-1,1),imu+gps)->pitch(-1,1)
    // throttle = vertical_vel(throttle(-1,1),realvel)->throttle(0,1)
    rcNormData.throttle =
        velocity_vertical_.step(rcNormData.throttle, baroData_.vertical_vel);
    // yaw = angular_vel(yaw(-1,1),brujula)->yaw(-1,1)

  case 1: // Pass Throw
    if (rcNormData.throttle < 0)
      rcNormData.throttle = 0;

    rcNormData.throttle = (rcNormData.throttle * 2) - 1;
    break;

  default: // Default values
    rcNormData.roll = 0;
    rcNormData.pitch = 0;
    rcNormData.throttle = -1;
    rcNormData.yaw = 0;
    rcNormData.armed = false;
    break;
  }

  return rcNormData;
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
