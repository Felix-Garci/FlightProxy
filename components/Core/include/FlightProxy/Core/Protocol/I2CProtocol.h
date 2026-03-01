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
    size_t total_size = 4;
    if (!packet->is_read) {
      total_size += packet->payload.size();
    }

    std::vector<uint8_t> buffer;
    buffer.reserve(total_size);

    buffer.push_back(packet->device_addr);
    buffer.push_back(packet->reg_addr);
    buffer.push_back(packet->is_read ? 1 : 0);
    buffer.push_back(packet->payload_len);

    if (!packet->is_read) {
      buffer.insert(buffer.end(), packet->payload.begin(),
                    packet->payload.end());
    }

    return buffer;
  }
};

class I2CDecoder : public IDecoderT<I2CPacket> {
public:
  void feed(const uint8_t *data, size_t len) override {
    if (len < 4)
      return;

    auto packet = std::make_unique<I2CPacket>();

    packet->device_addr = data[0];
    packet->reg_addr = data[1];
    packet->is_read = static_cast<bool>(data[2]);
    packet->payload_len = data[3];

    if (len >= static_cast<size_t>(4 + packet->payload_len)) {
      packet->payload.assign(data + 4, data + 4 + packet->payload_len);
    }

    if (onPacketHandler_) {
      onPacketHandler_(std::move(packet));
    }
  }

  void onPacket(
      std::function<void(std::unique_ptr<const I2CPacket>)> handler) override {
    onPacketHandler_ = std::move(handler);
  }

  void reset() override {}

private:
  std::function<void(std::unique_ptr<const I2CPacket>)> onPacketHandler_;
};

} // namespace Protocol
} // namespace Core
} // namespace FlightProxy
