#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include <functional>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace Command {
namespace Commands {

template <typename PacketT>
class MSP_Set_Hover
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Set_Hover<PacketT>> {
public:
  MSP_Set_Hover(std::function<void(uint16_t)> setterHover) {
    setterHover_ = setterHover;
    setterHover_(1000);
  }

  ~MSP_Set_Hover() {}

  int getID() override { return (int)Core::Protocol::MSP_SET_HOVER; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {
    uint16_t hover = 1000;
    if (packet->payload.size() >= 2)
      hover =
          packet->payload[0] | (static_cast<uint16_t>(packet->payload[1]) << 8);

    setterHover_(hover);

    auto replyPacket =
        std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{});
    reply(std::move(replyPacket));
  }

private:
  std::function<void(uint16_t)> setterHover_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
