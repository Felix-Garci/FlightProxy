#pragma once

#include "FlightProxy/Core/Utils/Logger.h"

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

namespace FlightProxy {
namespace Core {
namespace Protocol {

class GPSEncoder : public IEncoderT<GPSData> {
public:
  std::vector<uint8_t> encode(std::unique_ptr<const GPSData> packet) override {
    return {0};
  }
};

class GPSDecoder : public IDecoderT<GPSData> {
private:
  enum class State { IDLE, ID, DATA, CHECKSUM };

  State state_ = State::IDLE;
  uint8_t calculated_checksum_ = 0;
  uint8_t comma_count_ = 0;
  uint8_t field_idx_ = 0;
  char field_buf_[16];
  char id_buf_[6];

  bool is_rmc_ = false;
  bool is_valid_ = false;

  std::unique_ptr<GPSData> current_gps_ = std::make_unique<GPSData>();

  float nmeaToDeg(float nmea) {
    int degrees = (int)(nmea / 100);
    float minutes = nmea - (degrees * 100);
    return degrees + (minutes / 60.0f);
  }

  void processField() {
    field_buf_[field_idx_] = '\0';

    switch (comma_count_) {
    case 2: // Status: A=Valid, V=Invalid
      is_valid_ = (field_buf_[0] == 'A');
      break;
    case 3: // Latitude
      current_gps_->latitude = nmeaToDeg(std::atof(field_buf_));
      break;
    case 4: // N/S
      if (field_buf_[0] == 'S')
        current_gps_->latitude *= -1.0f;
      break;
    case 5: // Longitude
      current_gps_->longitude = nmeaToDeg(std::atof(field_buf_));
      break;
    case 6: // E/W
      if (field_buf_[0] == 'W')
        current_gps_->longitude *= -1.0f;
      break;
    case 7: // Speed (Knots to m/s)
      current_gps_->speed = std::atof(field_buf_) * 0.514444f;
      break;
    case 8: // Heading (Course)
      current_gps_->heading = std::atof(field_buf_);
      break;
    }
    field_idx_ = 0;
  }

public:
  void parse(uint8_t byte) {
    if (byte == '$') {
      state_ = State::ID;
      field_idx_ = 0;
      calculated_checksum_ = 0;
      comma_count_ = 0;
      is_rmc_ = false;
      return;
    }

    if (byte == '*') {
      processField();
      state_ = State::CHECKSUM;
      return;
    }

    if (state_ != State::CHECKSUM) {
      calculated_checksum_ ^= byte;
    }

    switch (state_) {
    case State::ID:
      id_buf_[field_idx_++] = byte;
      if (field_idx_ == 5) {
        // Buscamos "RMC" en los últimos 3 caracteres del ID (GPRMC, GNRMC...)
        if (id_buf_[2] == 'R' && id_buf_[3] == 'M' && id_buf_[4] == 'C') {
          is_rmc_ = true;
          state_ = State::DATA;
        } else {
          state_ = State::IDLE;
        }
        field_idx_ = 0;
      }
      break;

    case State::DATA:
      if (byte == ',') {
        processField();
        comma_count_++;
      } else if (field_idx_ < sizeof(field_buf_) - 1) {
        field_buf_[field_idx_++] = byte;
      }
      break;

    case State::CHECKSUM:
      if (is_rmc_ && is_valid_ && onPacketHandler_) {
        onPacketHandler_(std::make_unique<const GPSData>(*current_gps_));
        // FP_LOG_D("GPSProtocol", "%f.02 %f.02 %f.02 %f.02 ",
        //          current_gps_->latitude, current_gps_->longitude,
        //          current_gps_->heading, current_gps_->speed);
      }
      state_ = State::IDLE;
      break;

    default:
      break;
    }
  }

  void feed(const uint8_t *data, size_t len) override {
    for (size_t i = 0; i < len; ++i) {
      parse(data[i]);
    }
  }

  void onPacket(
      std::function<void(std::unique_ptr<const GPSData>)> handler) override {
    onPacketHandler_ = std::move(handler);
  }

  void reset() override { state_ = State::IDLE; }

private:
  std::function<void(std::unique_ptr<const GPSData>)> onPacketHandler_;
};

} // namespace Protocol
} // namespace Core
} // namespace FlightProxy
