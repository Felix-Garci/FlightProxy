#pragma once
#include "ICommand.h"

namespace FlightProxy {
namespace AppLogic {
namespace Command {
using AutoComandHandler =
    std::function<std::vector<uint8_t>(const std::vector<uint8_t> &)>;

template <typename PacketT> class AutoCommand : public ICommand<PacketT> {
public:
  AutoCommand(int ID, AutoComandHandler handler) : ID_(ID), handler_(handler) {}

  ~AutoCommand() {}

  void execute(const std::unique_ptr<const PacketT> &packet,
               ReplyFunc<PacketT> reply) override {

    reply(std::make_unique<const PacketT>('<', (uint8_t)ID_,
                                          handler_(packet->payload)));
  }

  int getID() override { return (int)ID_; }

private:
  int ID_;
  AutoComandHandler handler_;
};

} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
