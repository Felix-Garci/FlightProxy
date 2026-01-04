#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <functional>
#include <memory>
#include <string>

namespace FlightProxy {
namespace AppLogic {
namespace Command {
namespace Commands {

template <typename PacketT>
class MSP_Set_CtrActive
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Set_CtrActive<PacketT>> {
public:
  MSP_Set_CtrActive(std::function<void(std::string)> activeControlSetter) {
    activeControlSetter_ = activeControlSetter;
  }

  ~MSP_Set_CtrActive() {}

  int getID() override { return (int)Core::Protocol::MSP_SET_CTRLACTIVE; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {
    std::string activeControl =
        std::string(packet->payload.begin(), packet->payload.end());
    FP_LOG_D("Main", activeControl.c_str());
    activeControlSetter_(activeControl);

    auto replyPacket =
        std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{});
    reply(std::move(replyPacket));
  }

private:
  std::function<void(std::string)> activeControlSetter_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
