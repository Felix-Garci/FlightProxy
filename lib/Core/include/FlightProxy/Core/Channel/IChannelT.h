#pragma once
#include <functional>

namespace FlightProxy
{
    namespace Core
    {
        namespace Channel
        {
            template <typename PacketT>
            class IChannelT
            {
            public:
                virtual ~IChannelT() = default;

                virtual void open() = 0;
                virtual void close() = 0;
                virtual void sendPacket(std::unique_ptr<const PacketT> packet) = 0;

                // Callbacks que el usuario (dueño del channel) puede suscribir
                std::function<void(std::unique_ptr<const PacketT>)> onPacket;
                std::function<void()> onOpen;
                std::function<void()> onClose;
            };
        }
    }
}