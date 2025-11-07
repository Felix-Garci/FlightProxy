#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
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
                class MSP_BasicRead_Command : public ICommand<PacketT>
                {
                public:
                    MSP_BasicRead_Command(std::function<int(void)> lector) {}
                    int getID() override { return 0; }

                    void execute(const PacketT &packet) override
                    {
                                        }

                private:
                    std::function<int(void)> lector_;
                };
            }
        }
    }
}