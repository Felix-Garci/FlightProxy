#pragma once

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            template <typename PacketT>
            using ReplyFunc = std::function<void(std::unique_ptr<const PacketT>)>;

            template <typename PacketT>
            class ICommand
            {
            public:
                virtual ~ICommand() = default;
                virtual void execute(std::unique_ptr<const PacketT> packet, ReplyFunc<PacketT> reply) = 0;
                virtual int getID() = 0;
            };
        }
    }
}