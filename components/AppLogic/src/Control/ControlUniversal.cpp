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
  yawSetter_ = bb->registrarProductor<Core::YawData>(ID_YAW_Data);

  // Controles
  auto vlci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_LATVEL_IN);
  auto vlct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_LATVEL_OUT);
  velocity_lateral_.init(vlci, vlct);

  auto vfci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_FRNTVEL_IN);
  auto vfct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_FRNTVEL_OUT);
  velocity_frontal_.init(vfci, vfct);

  auto pvci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_VERTPOS_IN);
  auto pvct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_VERTPOS_OUT);
  position_vertical_.init(pvci, pvct);
  auto vvci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_VERTVEL_IN);
  auto vvct = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_VERTVEL_OUT);
  velocity_vertical_.init(vvci, vvct);

  auto vaci = bb->registrarConsumidor<Core::PidCtrlIn>(ID_CTRL_ANGVEL_IN);
  auto vact = bb->registrarProductor<Core::PidCtrlOut>(ID_CTRL_ANGVEL_OUT);
  velocity_angular_.init(vaci, vact);
}
Core::RCNORMData ControlUniversal::step(uint8_t ctrlLevel,
                                        Core::RCNORMData rcNormData, float dt) {
  levelData_ = levelGetter_();

  // paso 1
  attitudeData_ = attitudeGetter_();
  magData_ = magGetter_();

  attitudeData_.yaw = tilt_compensation(attitudeData_, magData_);

  // paso 2
  imuData_ = imuGetter_();

  attitudeData_.yaw_rate = yaw_rate_transformation(attitudeData_, imuData_);
  yawData_.yaw_rate = attitudeData_.yaw_rate;

  // paso 3
  attitudeData_.yaw = sensor_fusion_realyaw(attitudeData_, dt);
  yawData_.yaw = attitudeData_.yaw;

  // paso 4
  gpsData_ = gpsGetter_();
  baroData_ = baroGetter_();

  velData_ = velocity_transform(attitudeData_, gpsData_, baroData_);

  velSetter_(velData_);
  yawSetter_(yawData_);

  ctrlLevel = ctrlLevel > levelData_ ? levelData_ : ctrlLevel;

  if (prevLevel_ != ctrlLevel) {
    FP_LOG_D("ControlUniversal", "ctrl lvl from %d => %d", prevLevel_,
             ctrlLevel);

    velocity_lateral_.reset();

    velocity_frontal_.reset();

    velocity_vertical_.reset();
    position_vertical_.reset();

    velocity_angular_.reset();

    cold_start_yaw_estimate_ = true;

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
    {
      float cy = std::cos(attitudeData_.yaw);
      float sy = std::sin(attitudeData_.yaw);
      float ref_n = rcNormData.pitch; // referencia de velocidad norte (-1,1)
      float ref_e = rcNormData.roll;  // referencia de velocidad este (-1,1)

      rcNormData.pitch = ref_n * cy + ref_e * sy;  // -> v_rel_x (frontal)
      rcNormData.roll = -ref_n * sy + ref_e * cy;  // -> v_rel_y (lateral)
    }

    [[fallthrough]];
  case 2: // Relative vel + w

    // roll=lateral_vel(roll(-1,1),imu+gps)->roll(-1,1)
    rcNormData.roll =
        velocity_lateral_.step(rcNormData.roll, velData_.v_rel_y, dt);

    // pitch=frontal_vel(pitch(-1,1),imu+gps)->pitch(-1,1)
    rcNormData.pitch =
        velocity_frontal_.step(rcNormData.pitch, velData_.v_rel_x, dt);

    // throttle = vertical_vel(throttle(-1,1),realvel)->throttle(0,1)
    rcNormData.throttle = velocity_vertical_.step(rcNormData.throttle,
                                                  baroData_.vertical_vel, dt);

    // yaw = angular_vel(yaw(-1,1),brujula)->yaw(-1,1)
    // rcNormData.yaw =
    //    velocity_angular_.step(rcNormData.yaw, attitudeData_.yaw_rate, dt);

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
  // FP_LOG_D("Universal", "x=%.4f y=%.4f", velData_.v_rel_x, velData_.v_rel_y);
  // FP_LOG_D("Universal", "r=%8.4f p=%8.4f y=%8.4f", attitudeData_.roll,
  //          attitudeData_.pitch, attitudeData_.yaw);

  return rcNormData;
}

// Devuelve: yaw_mag absoluto en radianes
float ControlUniversal::tilt_compensation(Core::AttitudeData attitudeData,
                                          Core::MagData magData) {
  float pitch = attitudeData.pitch;
  float roll = attitudeData.roll;

  float xh = magData.mag_x * std::cos(pitch) +
             magData.mag_y * std::sin(roll) * std::sin(pitch) +
             magData.mag_z * std::cos(roll) * std::sin(pitch);

  float yh = magData.mag_y * std::cos(roll) - magData.mag_z * std::sin(roll);

  return std::atan2(-yh, xh);
}

// Devuelve: yaw_rate (derivada pura de Euler) en radianes/segundo
float ControlUniversal::yaw_rate_transformation(Core::AttitudeData attitudeData,
                                                Core::IMUData imuData) {
  float pitch = attitudeData.pitch;
  float roll = attitudeData.roll;

  return (imuData.gyro_y * std::sin(roll) + imuData.gyro_z * std::cos(roll)) /
         std::cos(pitch);
}

// Devuelve: yaw_real (Yaw absoluto sin ruido) en radianes
float ControlUniversal::sensor_fusion_realyaw(Core::AttitudeData attitudeData,
                                              float dt) {
  if (cold_start_yaw_estimate_) {
    yaw_estimate_ = attitudeData_.yaw; // Forzamos la convergencia instantánea
    cold_start_yaw_estimate_ = false;
  }

  // 1. Predicción rápida (Giroscopio)
  yaw_estimate_ += (attitudeData.yaw_rate * dt);

  //  Mantener estimación entre -PI y PI
  if (yaw_estimate_ > M_PI)
    yaw_estimate_ -= 2.0 * M_PI;
  if (yaw_estimate_ < -M_PI)
    yaw_estimate_ += 2.0 * M_PI;

  // 2. Cálculo del Error (Magnetómetro - Predicción)
  // Se asume que attitudeData.yaw ya trae el valor de tilt_compensation()
  float e = attitudeData.yaw - yaw_estimate_;

  // Forzamos el error a tomar el camino más corto
  if (e > M_PI)
    e -= 2.0 * M_PI;
  if (e < -M_PI)
    e += 2.0 * M_PI;

  // 3. Corrección lenta (Magnetómetro).
  yaw_estimate_ += (1.0 * e * dt);

  return yaw_estimate_;
}

// Devuelve: Velocidades en el Marco del Dron (FRD)
Core::VelocityData
ControlUniversal::velocity_transform(Core::AttitudeData attitudeData,
                                     Core::GPSData gpsData,
                                     Core::BaroData baroData) {
  // FP_LOG_D("DEBUG_MATRIX",
  //          "Angulos IN: R=%.3f, P=%.3f, Y=%.3f | GPS_H=%.3f, GPS_S=%.3f",
  //          attitudeData.roll, attitudeData.pitch, attitudeData.yaw,
  //          gpsData.heading, gpsData.speed);

  // 1. Preparación de variables trigonométricas
  float cp = std::cos(attitudeData.pitch);
  float sp = std::sin(attitudeData.pitch);
  float cr = std::cos(attitudeData.roll);
  float sr = std::sin(attitudeData.roll);
  float cy = std::cos(attitudeData.yaw); // Usamos el Yaw Real ya filtrado
  float sy = std::sin(attitudeData.yaw);

  // 2. Construcción del Vector de Velocidad Inercial (NED)
  float vn = 0.0f;
  float ve = 0.0f;

  if (gpsData.speed > 0.01f) {
    vn = gpsData.speed * std::cos(gpsData.heading);
    ve = gpsData.speed * std::sin(gpsData.heading);
  }

  // Invertimos la velocidad vertical
  // (si baroData asume UP positivo)
  float vd = -baroData.vertical_vel;

  // 3. Multiplicación Matricial 3D (R_I^B * V_I)
  Core::VelocityData velData;
  velData.v_abs_x = vn;
  velData.v_abs_y = ve;
  velData.v_abs_z = baroData.vertical_vel;

  // Eje X (Morro): v_x = VN(cosT*cosY) + VE(cosT*sinY) - VD(sinT)
  velData.v_rel_x = vn * (cp * cy) + ve * (cp * sy) - vd * sp;

  // Eje Y (Ala Derecha)
  velData.v_rel_y = vn * (sr * sp * cy - cr * sy) +
                    ve * (sr * sp * sy + cr * cy) + vd * (sr * cp);

  // Eje Z (Panza)
  velData.v_rel_z = vn * (cr * sp * cy + sr * sy) +
                    ve * (cr * sp * sy - sr * cy) + vd * (cr * cp);

  return velData;
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
