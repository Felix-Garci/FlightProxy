#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <cstring>
#include <functional>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace Command {
namespace Commands {

template <typename PacketT>
class MSP_Set_PIDCts
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Set_PIDCts<PacketT>> {
public:
  MSP_Set_PIDCts(std::function<void(Core::ControlPIDCts)> setterPIDCts) {
    setterPIDCts_ = setterPIDCts;
    // Inicializamos a 000
    setterPIDCts_({0, 0, 0});
  }

  ~MSP_Set_PIDCts() {}

  int getID() override { return (int)Core::Protocol::MSP_SET_PIDCST; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {
    Core::ControlPIDCts cts;

    cts.p = 0;
    cts.i = 0;
    cts.d = 0;

    // float = 4 uint8_t
    if (packet->payload.size() >= 12) {
      memcpy(&cts.p, &packet->payload[0], sizeof(float));
      memcpy(&cts.i, &packet->payload[4], sizeof(float));
      memcpy(&cts.d, &packet->payload[8], sizeof(float));
    }

    FP_LOG_D("MSP_SET_PIDCST", "%.2f %.2f %.2f", cts.p, cts.i, cts.d);

    setterPIDCts_(cts);

    auto replyPacket =
        std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{});
    reply(std::move(replyPacket));
  }

private:
  std::function<void(Core::ControlPIDCts)> setterPIDCts_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
