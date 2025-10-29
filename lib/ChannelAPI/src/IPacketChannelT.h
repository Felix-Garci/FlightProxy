#pragma once
/*
interface IPacketChannelT<PacketT>{
    ' Atributos
    {field}+onPacket: std::function<void(const PacketT&)>
    {field}+onOpen: std::function<void()>
    {field}+onClose: std::function<void()>

    ' Métodos
    +~IPacketChannelT()
    +open(): void
    +close(): void
    +sendPacket(packet:const PacketT&): void
}
*/
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

            std::function<void(const PacketT &)> onPacket;
            std::function<void()> onOpen;
            std::function<void()> onClose;
        };
    }
}