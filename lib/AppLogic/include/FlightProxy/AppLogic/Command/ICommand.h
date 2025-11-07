#pragma once

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            template <typename PacketT>
            class ICommand
            {
            public:
                virtual ~ICommand() = default;
                virtual void execute(const PacketT &packet) = 0;
                virtual int getID() = 0;
            };
        }
    }
}