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
class MSP_Get_PIDVals
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Get_PIDVals<PacketT>> {
public:
  MSP_Get_PIDVals(std::function<Core::ControlPIDVals(void)> getterPIDVals) {
    getterPIDVals_ = getterPIDVals;
  }

  ~MSP_Get_PIDVals() {}

  int getID() override { return (int)Core::Protocol::MSP_GET_PIDVALS; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {

    Core::ControlPIDVals vals = getterPIDVals_();

    auto replyPacket = std::make_unique<const PacketT>(
        '<', 1,
        std::vector<uint8_t>((uint8_t *)&vals,
                             (uint8_t *)&vals + sizeof(Core::ControlPIDVals)));
    reply(std::move(replyPacket));
  }

private:
  std::function<Core::ControlPIDVals(void)> getterPIDVals_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
