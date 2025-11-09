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
                class MSP_BasicWrite_Command : public ICommand<PacketT>,
                                               public std::enable_shared_from_this<MSP_BasicRead_Command<PacketT>>
                {
                public:
                    MSP_BasicWrite_Command() {}
                    int getID() override { return 2; }

                    void execute(const PacketT &packet, ReplyFunc<PacketT> reply) override
                    {
                        FP_LOG_W("Pewrro", "Estamos a punto de responder dentro del comando");
                        reply(packet);
                    }

                private:
                };
            }
        }
    }
}