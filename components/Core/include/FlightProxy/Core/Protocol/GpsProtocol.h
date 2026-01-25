#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace FlightProxy {
namespace Core {
namespace Protocol {

class I2CEncoder : public IEncoderT<GPSData> {
public:
  std::vector<uint8_t> encode(std::unique_ptr<const GPSData> packet) override {
    return {0};
  }
};

class I2CDecoder : public IDecoderT<GPSData> {
private:
  enum class State {
    IDLE,
    ID,
    MSG,

    STAR,
    CHECKSUM,
  };
  State state_ = State::IDLE;
  uint16_t current_chechsum_ = 0;
  uint8_t GPSH_EADER = '$';

public:
  void parse(uint8_t byte) {
    switch (state_) {

    case State::IDLE:
      if (byte == GPSH_EADER)
        state_ = State::ID;
      break;
    case State::ID:
      break;
    case State::MSG:
      break;
    case State::STAR:
      break;
    case State::CHECKSUM:
      break;
    }
    void feed(const uint8_t *data, size_t len) override {}

    void onPacket(std::function<void(std::unique_ptr<const GPSData>)> handler)
        override {
      onPacketHandler_ = std::move(handler);
    }

    void reset() override {}

  private:
    std::function<void(std::unique_ptr<const GPSData>)> onPacketHandler_;
  };

} // namespace Protocol
} // namespace Core
} // namespace FlightProxy
