#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <functional>

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            namespace Commands
            {

                template <typename PacketT>
                class MSP_BasicRead_Command : public ICommand<PacketT>,
                                              public std::enable_shared_from_this<MSP_BasicRead_Command<PacketT>>
                {
                public:
                    MSP_BasicRead_Command() {}
                    int getID() override { return 1; }

                    void execute(std::unique_ptr<const PacketT> packet, ReplyFunc<PacketT> reply) override
                    {
                        FP_LOG_W("Read command", "Estamos a punto de responder dentro del comando");
                        // hacer cosas con la lecutra de consumer

                        // montar un packet
                        auto replyPacket = std::make_unique<const PacketT>('<', 1, std::vector<uint8_t>{0x10, 0x20, 0x30, 0x40});

                        reply(std::move(replyPacket));
                    }

                private:
                };
            }
        }
    }
}