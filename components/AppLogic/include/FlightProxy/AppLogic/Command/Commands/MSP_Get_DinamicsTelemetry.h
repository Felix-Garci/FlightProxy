#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include <cstring>
#include <functional>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace Command {
namespace Commands {

template <typename PacketT>
class MSP_Get_DinamicsTelemetry
    : public ICommand<PacketT>,
      public std::enable_shared_from_this<MSP_Get_DinamicsTelemetry<PacketT>> {
public:
  MSP_Get_DinamicsTelemetry(
      std::function<Core::RCData(void)> getterRCOutput,
      std::function<Core::BaroData(void)> getterBaroData) {

    getterrcoutput_ = getterRCOutput;
    getterbarodata_ = getterBaroData;
  }
  ~MSP_Get_DinamicsTelemetry() {}
  int getID() override {
    return (int)Core::Protocol::MSP_GET_DINAMICSTELEMETRY;
  }

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {

    Core::RCData rc = getterrcoutput_();
    Core::BaroData baro = getterbarodata_();

    float throttle = (float)rc.throttle;
    float alt = baro.altitude;
    float vel = baro.vertical_vel;

    // FP_LOG_D("DinamicTelemetry", "%.2f  %.2f  %.2f ", throttle, alt, vel);

    std::vector<uint8_t> buffer(12);
    std::memcpy(&buffer[0], &throttle, sizeof(throttle));
    std::memcpy(&buffer[4], &alt, sizeof(alt));
    std::memcpy(&buffer[8], &vel, sizeof(vel));

    auto replyPacket = std::make_unique<const PacketT>('<', 1, buffer);

    reply(std::move(replyPacket));
  }

private:
  std::function<Core::RCData(void)> getterrcoutput_;
  std::function<Core::BaroData(void)> getterbarodata_;
};
} // namespace Commands
} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
