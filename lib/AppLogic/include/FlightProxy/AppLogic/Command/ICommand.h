#pragma once

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            template <typename PacketT>
            using ReplyFunc = std::function<void(const PacketT &)>;

            template <typename PacketT>
            class ICommand
            {
            public:
                virtual ~ICommand() = default;
                virtual void execute(const PacketT &packet, ReplyFunc<PacketT> reply) = 0;
                virtual int getID() = 0;
            };
        }
    }
}