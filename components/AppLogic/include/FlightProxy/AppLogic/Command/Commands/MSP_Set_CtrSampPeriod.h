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
class MSP_Set_CtrSampPeriod
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Set_CtrSampPeriod<PacketT>> {
public:
  MSP_Set_CtrSampPeriod(std::function<void(uint64_t)> samplePeriodMsSetter) {
    samplePeriodMsSetter_ = samplePeriodMsSetter;
  }

  ~MSP_Set_CtrSampPeriod() {}

  int getID() override { return (int)Core::Protocol::MSP_SET_SAMPLPERIMS; }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {
    uint64_t periodMs = 0;
    for (int i = 0; i < packet->payload.size(); i++) {
      periodMs |= (uint64_t)packet->payload[i] << (8 * i);
    }
    samplePeriodMsSetter_(periodMs);

    auto replyPacket =
        std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{});
    reply(std::move(replyPacket));
  }

private:
  std::function<void(uint64_t)> samplePeriodMsSetter_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
