#include "FlightProxy/AppLogic/Control/ControlUniversal.h"

#include "FlightProxy/AppLogic/AlmazenFlexibleID.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <math.h>

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
  attitudeGetter_ =
      bb->registrarConsumidor<Core::AttitudeData>(ID_ATTITUDE_Data);
  magGetter_ = bb->registrarConsumidor<Core::MagData>(ID_MAG_Data);

  // Data Setters
  velSetter_ = bb->registrarProductor<Core::VelocityData>(ID_VEL_Data);

  // Controles
  auto vvci =
      bb->registrarConsumidor<Core::PidCtrlVertVelIn>(ID_CTRL_VERTVEL_IN);
  auto vvct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_VERTVEL_OUT);
  velocity_vertical_.init(vvci, vvct);

  auto pvci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_VERTPOS_IN);
  auto pvct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_VERTPOS_OUT);
  position_vertical_.init(pvci, pvct);

  auto vfci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_FRNTVEL_IN);
  auto vfct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_FRNTVEL_OUT);
  velocity_frontal_.init(vfci, vfct);
}
Core::RCNORMData ControlUniversal::step(uint8_t ctrlLevel,
                                        Core::RCNORMData rcNormData, float dt) {
  levelData_ = levelGetter_();

  attitudeData_ = attitudeGetter_();
  magData_ = magGetter_();
  magCompensation();
  imuData_ = imuGetter_();
  eulerProyection(dt);
  gpsData_ = gpsGetter_();
  float vx, vy = 0;
  vGps2DroneRef(vx, vy);
  velData_.v_rel_x = vx;
  velData_.v_rel_y = vy;
  velSetter_(velData_);

  baroData_ = baroGetter_();

  ctrlLevel = ctrlLevel > levelData_ ? levelData_ : ctrlLevel;

  if (prevLevel_ != ctrlLevel) {
    FP_LOG_D("ControlUniversal", "ctrl lvl from %d => %d", prevLevel_,
             ctrlLevel);

    velocity_vertical_.reset();
    position_vertical_.reset();

    velocity_frontal_.reset();
    prevLevel_ = ctrlLevel;
  }

  switch (ctrlLevel) {
  case 5: // Path planner

    // x,y,z,grad = pathplaner(newxWP)

    [[fallthrough]];
  case 4: // Coordinates + orientation

    // roll,pitch=posicion_horizontal(x,y,gps)->(-1,1)(-1,1)/horizontal_abs_vel_ref
    // throttle=posicion_vertical(z,baro)->(-1,1)/vertical_vel_ref

    rcNormData.throttle =
        position_vertical_.step(rcNormData.throttle, baroData_.altitude, dt);

    // yaw=horientazion(abs_grad,brujula)->(-1,1)/angular_vel_ref

    [[fallthrough]];
  case 3: // Absolute vel + w

    // roll,pitch=reff_transform(roll(-1,1),pitch(-1,1),brujula)->(-1,1)(-1,1)/horizontal_rel_vel

    [[fallthrough]];
  case 2: // Relative vel + w

    // roll=lateral_vel(roll(-1,1),imu+gps)->roll(-1,1)
    // pitch=frontal_vel(pitch(-1,1),imu+gps)->pitch(-1,1)

    rcNormData.pitch = velocity_frontal_.step(rcNormData.pitch, vx, dt);

    // throttle = vertical_vel(throttle(-1,1),realvel)->throttle(0,1)
    rcNormData.throttle = velocity_vertical_.step(rcNormData.throttle,
                                                  baroData_.vertical_vel, dt);

    // yaw = angular_vel(yaw(-1,1),brujula)->yaw(-1,1)

    [[fallthrough]];
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
  // FP_LOG_D("Universal", "x=%.4f y=%.4f", vx, vy);
  //   FP_LOG_D("Universal", "r=%8.4f p=%8.4f y=%8.4f", attitudeData_.roll,
  //           attitudeData_.pitch, attitudeData_.yaw);

  return rcNormData;
}

void ControlUniversal::magCompensation() {
  float pitch = attitudeData_.pitch * std::numbers::pi / 180.0;
  float roll = attitudeData_.roll * std::numbers::pi / 180.0;

  float xh = magData_.mag_x * cos(pitch) +
             magData_.mag_y * sin(roll) * sin(pitch) +
             magData_.mag_z * cos(roll) * sin(pitch);

  float yh = magData_.mag_y * cos(roll) - magData_.mag_z * sin(roll);

  magData_.mag_x = xh;
  magData_.mag_y = yh;

  float yawMag = atan2(-yh, xh);
  float yawGrados = yawMag * 180.0 / std::numbers::pi;

  attitudeData_.yaw = yawGrados;
}

void ControlUniversal::eulerProyection(float dt) {
  float pitch = attitudeData_.pitch * std::numbers::pi / 180.0;
  float roll = attitudeData_.roll * std::numbers::pi / 180.0;

  float gyroY_rad = imuData_.gyro_y * (std::numbers::pi / 180.0f);
  float gyroZ_rad = imuData_.gyro_z * (std::numbers::pi / 180.0f);

  float yawRate = (gyroY_rad * sin(roll) + gyroZ_rad * cos(roll)) / cos(pitch);

  this->yawRealRad_ += yawRate * dt;

  float yawMagRad = attitudeData_.yaw * std::numbers::pi / 180.0;

  float error = yawMagRad - yawRealRad_;

  if (error > std::numbers::pi)
    error -= 2.0 * std::numbers::pi;
  if (error < -std::numbers::pi)
    error += 2.0 * std::numbers::pi;

  yawRealRad_ += 4.0f * error * dt;

  if (yawRealRad_ > std::numbers::pi)
    yawRealRad_ -= 2.0 * std::numbers::pi;
  else if (yawRealRad_ < -std::numbers::pi)
    yawRealRad_ += 2.0 * std::numbers::pi;

  attitudeData_.yaw = yawRealRad_ * 180.0 / std::numbers::pi;
}

void ControlUniversal::vGps2DroneRef(float &vx, float &vy) {
  float yawRad = attitudeData_.yaw * std::numbers::pi / 180.0;
  float headingRad = gpsData_.heading * std::numbers::pi / 180.0;

  float diffAngle = headingRad - yawRad;

  vx = gpsData_.speed * sin(diffAngle);
  vy = -1.0f * gpsData_.speed * cos(diffAngle);
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
