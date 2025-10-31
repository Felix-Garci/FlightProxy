#include "FlightProxy/Core/Protocol/MspProtocol.h"

#include "FlightProxy/Channel/UartTransportManager.h"
#include "FlightProxy/Transport/SimpleUart.h"

#include "driver/uart.h"
#include "driver/gpio.h"

extern "C" void app_main(void)
{
    FlightProxy::Channel::UartTransportManagerT<FlightProxy::Core::MspPacket> uartManager{};
    FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *Channel = nullptr;

    uartManager.onNewChannel = [&](FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *channel)
    {
        Channel = channel;
    };

    uartManager.start(
        []() -> FlightProxy::Core::Protocol::IDecoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspDecoder();
        },
        []() -> FlightProxy::Core::Protocol::IEncoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspEncoder();
        },
        []() -> FlightProxy::Core::Transport::ITransport *
        {
            return new FlightProxy::Transport::SimpleUart(
                UART_NUM_0,
                GPIO_NUM_1,
                GPIO_NUM_3,
                115200);
        });

    Channel->open();
}

/*#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Channel/UartTransportManager.h"
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Channel/IPacketChannelT.h"

#include "FlightProxy/Core/Utils/Logger.h"

#include "driver/uart.h"
#include "driver/gpio.h"

extern "C" void app_main(void)
{
    FlightProxy::Channel::UartTransportManagerT<FlightProxy::Core::MspPacket> uartManager(
        UART_NUM_0,
        GPIO_NUM_1,
        GPIO_NUM_3,
        115200);

    FlightProxy::Channel::IPacketChannelT<FlightProxy::Core::MspPacket> *packetChannel = nullptr;

    uartManager.onNewChannel = [&](FlightProxy::Channel::IPacketChannelT<FlightProxy::Core::MspPacket> *channel)
    {
        packetChannel = channel;
    };

    uartManager.start(
        []() -> FlightProxy::Core::Protocol::IDecoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspDecoder();
        },
        []() -> FlightProxy::Core::Protocol::IEncoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspEncoder();
        });

    // Aquí podrías agregar lógica adicional para enviar/recibir paquetes usando packetChannel
    packetChannel->onPacket = [](const FlightProxy::Core::MspPacket &packet)
    {
        FP_LOG_I("Main", "Received MSP Packet: Command=%u, Payload Size=%zu", packet.command, packet.payload.size());
    };

    packetChannel->open();

    // Bucle principal (aquí solo como ejemplo, en un caso real usarías FreeRTOS tasks)
    uint8_t i = 0;
    while (true)
    {
        packetChannel->sendPacket(FlightProxy::Core::MspPacket{
            .direction = '>',
            .command = i++,
            .payload = {0x01, 0x02, 0x03}});
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/