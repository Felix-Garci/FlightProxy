#pragma once
#include <functional>

namespace FlightProxy
{
    namespace Channel
    {
        template <typename PacketT>
        class IPacketChannelT
        {
        public:
            virtual ~IPacketChannelT() = default;

            virtual void open() = 0;
            virtual void close() = 0;
            virtual void sendPacket(const PacketT &packet) = 0;

            // Callbacks que el usuario (dueño del channel) puede suscribir
            std::function<void(const PacketT &)> onPacket;
            std::function<void()> onOpen;
            std::function<void()> onClose;
        };
    }
}