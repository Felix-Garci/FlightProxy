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

class I2CEncoder : public IEncoderT<I2CPacket> {
public:
  std::vector<uint8_t>
  encode(std::unique_ptr<const I2CPacket> packet) override {
    std::vector<uint8_t> buffer(3);
    buffer[0] = packet->device_addr;
    buffer[1] = packet->reg_addr;
    buffer[2] = packet->payload_len;

    return buffer;
  }
};

class I2CDecoder : public IDecoderT<I2CPacket> {
public:
  void feed(const uint8_t *data, size_t len) {
    if (len < 3)
      return;

    auto packet = std::make_unique<I2CPacket>();

    packet->device_addr = data[0];
    packet->reg_addr = data[1];
    packet->payload_len = data[2];

    if (len >= 3 + packet->payload_len) {
      packet->payload.assign(data + 3, data + 3 + packet->payload_len);
    }

    if (onPacketHandler_) {
      onPacketHandler_(std::move(packet));
    }
  }

  void onPacket(std::function<void(std::unique_ptr<const I2CPacket>)> handler) {
    onPacketHandler_ = handler;
  }

  void reset() {}

private:
  std::function<void(std::unique_ptr<const I2CPacket>)> onPacketHandler_;
};

} // namespace Protocol
} // namespace Core
} // namespace FlightProxy
