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
                    MSP_BasicRead_Command(std::function<T(void)> consumer) : m_consumer(consumer) {}
                    int getID() override { return 1; }

                    void execute(const PacketT &packet, ReplyFunc<PacketT> reply) override
                    {
                        FP_LOG_W("Read command", "Estamos a punto de responder dentro del comando");
                        // hacer cosas con la lecutra de consumer

                        // montar un packet
                        reply(packet);
                    }

                private:
                    std::function<T(void)> m_consumer;
                };
            }
        }
    }
}