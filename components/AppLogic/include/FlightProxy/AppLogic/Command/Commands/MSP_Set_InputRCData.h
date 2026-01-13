#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <functional>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace Command {
namespace Commands {

template <typename PacketT>
class MSP_Set_InputRCData
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Set_InputRCData<PacketT>> {
public:
  MSP_Set_InputRCData(std::function<void(Core::RCData)> inputRCSetter) {
    inputRCSetter_ = inputRCSetter;
  }

  ~MSP_Set_InputRCData() {}

  int getID() override { return (int)Core::Protocol::MSP_SET_INPUTRC; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {

    Core::RCData recived;
    if (packet->payload.size() >= 12) {
      recived.roll =
          packet->payload[0] | (static_cast<uint16_t>(packet->payload[1]) << 8);
      recived.pitch =
          packet->payload[2] | (static_cast<uint16_t>(packet->payload[3]) << 8);
      recived.throttle =
          packet->payload[4] | (static_cast<uint16_t>(packet->payload[5]) << 8);
      recived.yaw =
          packet->payload[6] | (static_cast<uint16_t>(packet->payload[7]) << 8);
      recived.aux1 =
          packet->payload[8] | (static_cast<uint16_t>(packet->payload[9]) << 8);
      recived.aux2 = packet->payload[10] |
                     (static_cast<uint16_t>(packet->payload[11]) << 8);
      inputRCSetter_(recived);
    }

    auto replyPacket =
        std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{});
    reply(std::move(replyPacket));
  }

private:
  std::function<void(Core::RCData)> inputRCSetter_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
