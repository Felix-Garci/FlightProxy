#pragma once

#include "TypeSignature.h"
#include <array>
#include <cstdint>
#include <vector>

namespace FlightProxy {
namespace Core {

// Alias para evitar la coma dentro de las macros X
using AuxChannels = std::array<uint16_t, 8>;

struct MspPacket {
  char direction;
  uint16_t command;
  std::vector<uint8_t> payload;
  MspPacket(char dir, uint8_t cmd, std::vector<uint8_t> pld)
      : direction(dir), command(cmd), payload(std::move(pld)) {}
  MspPacket() {}
  ~MspPacket() {}
  MspPacket(const MspPacket &other)
      : direction(other.direction), command(other.command),
        payload(other.payload) {}
};

struct IBUSPacket {
  static constexpr size_t NUM_CHANNELS = 14;
  using ChannelsT = std::array<uint16_t, NUM_CHANNELS>;
  ChannelsT channels;
};

template <typename PacketT> struct PacketEnvelope {
  const PacketT *raw_packet_ptr;
  uint32_t channelId;
};

} // namespace Core
} // namespace FlightProxy

// --- DEFINICIÓN DE CAMPOS ---

#define RC_DATA_FIELDS(X)                                                      \
  X(uint16_t, roll)                                                            \
  X(uint16_t, pitch)                                                           \
  X(uint16_t, throttle)                                                        \
  X(uint16_t, yaw)                                                             \
  X(uint16_t, aux1)                                                            \
  X(uint16_t, aux2) X(FlightProxy::Core::AuxChannels, aux_channels)

REGISTER_FP_STRUCT(RCData, RC_DATA_FIELDS)

#define GPS_DATA_FIELDS(X)                                                     \
  X(double, latitude)                                                          \
  X(double, longitude) X(float, altitude) X(float, speed) X(float, heading)

REGISTER_FP_STRUCT(GPSData, GPS_DATA_FIELDS)

#define MAG_DATA_FIELDS(X) X(float, mag_x) X(float, mag_y) X(float, mag_z)

REGISTER_FP_STRUCT(MagData, MAG_DATA_FIELDS)

#define BARO_DATA_FIELDS(X) X(float, altitude) X(float, vertical_vel)

REGISTER_FP_STRUCT(BaroData, BARO_DATA_FIELDS)

#define IMU_DATA_FIELDS(X)                                                     \
  X(int16_t, accel_x)                                                          \
  X(int16_t, accel_y)                                                          \
  X(int16_t, accel_z) X(int16_t, gyro_x) X(int16_t, gyro_y) X(int16_t, gyro_z)

REGISTER_FP_STRUCT(IMUData, IMU_DATA_FIELDS)

#define CONTROL_PID_VALS_FIELDS(X)                                             \
  X(float, reference)                                                          \
  X(float, actual) X(float, output) X(float, p) X(float, i) X(float, d)

REGISTER_FP_STRUCT(ControlPIDVals, CONTROL_PID_VALS_FIELDS)

#define STATUS_DATA_FIELDS(X)                                                  \
  X(uint16_t, cycleTime)                                                       \
  X(uint16_t, i2c_errors)                                                      \
  X(uint16_t, sensors)                                                         \
  X(uint32_t, boxModeFlags)                                                    \
  X(uint8_t, currentProfileIndex)                                              \
  X(uint16_t, averageSystemLoadPercent)                                        \
  X(uint16_t, armingFlags) X(uint8_t, accCalibrationAxisFlags)

REGISTER_FP_STRUCT(StatusData, STATUS_DATA_FIELDS)

#define CONTROL_PID_CTS_FIELDS(X) X(float, p) X(float, i) X(float, d)
REGISTER_FP_STRUCT(ControlPIDCts, CONTROL_PID_CTS_FIELDS)
