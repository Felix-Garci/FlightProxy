#pragma once
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Channel/ChannelT.h"
/*
class TcpTransportManagerT<PacketT>{
    {field}+onNewChannel: std::function<void(IPacketChannelT<PacketT>*)>
    -port_: uint16_t
    -listenSocket_: netconn *
    -taskHandle_: TaskHandle_t
    -acceptTaskHandle_: TaskHandle_t
    {field}-encoderFactory_: std::function<IEncoderT<PacketT>*()>
    {field}-decoderFactory_: std::function<IDecoderT<PacketT>*()>

    -clients_: std::list<void*>

    +TcpTransportManagerT(port: uint16_t)
    +~TcpTransportManagerT()
    +start(df: std::function<IDecoderT<PacketT>*()>, ef: std::function<IEncoderT<PacketT>*()>): void
    -eventTask(): void
    -eventTaskAdapter(arg:void*): void
}
*/