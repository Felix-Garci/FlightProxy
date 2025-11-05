#include "FlightProxy/Core/Utils/Logger.h"       // La clase Logger de Core
#include "FlightProxy/PlatformESP32/EspLogger.h" // La implementación REAL
#include "AppFactory.h"

// 1. Creamos una instancia estática del logger REAL
static FlightProxy::PlatformESP32::EspLogger g_esp_logger;

// usos ejemplo
#include "FlightProxy/Connectivity/WifiManager.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Transport/SimpleTCP.h"

#include "FlightProxy/Channel/ChannelServer.h"
#include "FlightProxy/Transport/ListenerTCP.h"

#include "FlightProxy/Transport/SimpleUDP.h"

extern "C" void app_main(void)
{
    // 2. ¡PRIMERA LÍNEA! Inyectamos el logger en el singleton de Core.
    FlightProxy::Core::Utils::Logger::setInstance(g_esp_logger);

    FP_LOG_I("main", "Logger inicializado.");

    FlightProxy::Connectivity::WiFiManager wifiManager;

    // 2. Conectar (esto es bloqueante)
    // CAMBIA ESTO POR TUS CREDENCIALES
    FP_LOG_I("MAIN", "Intentando conectar a WiFi...");
    bool connected = wifiManager.connect("Sup", "rrrrrrrr");

    if (connected)
    {
        FP_LOG_I("MAIN", "¡WiFi conectado exitosamente!");
        // Aquí puedes iniciar otras tareas que dependan de la red
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); // Obtiene el netif de la estación

        if (netif)
        {
            esp_netif_get_ip_info(netif, &ip_info);
            FP_LOG_I("MAIN", "--- DEBUG DE RED ---");
            FP_LOG_I("MAIN", "Mi IP:      " IPSTR, IP2STR(&ip_info.ip));
            FP_LOG_I("MAIN", "Mi Netmask: " IPSTR, IP2STR(&ip_info.netmask));
            FP_LOG_I("MAIN", "Mi Gateway: " IPSTR, IP2STR(&ip_info.gw));
            FP_LOG_I("MAIN", "----------------------");
        }
    }
    else
    {
        FP_LOG_E("MAIN", "Falló la conexión a WiFi.");
        // Manejar el fallo (quizás reiniciar el ESP)
    }

    // Resto de app

    enum class TransportMode : int
    {
        TcpClient = 0,
        TcpServer = 1,
        UdpServer = 2
    };

    // Uso
    TransportMode mode = TransportMode::UdpServer;

    if (mode == TransportMode::TcpClient)
    {
        auto transport = std::make_shared<FlightProxy::Transport::SimpleTCP>("10.26.145.193", 12345);
        auto encoder = std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
        auto decoder = std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();
        auto tcp_channel = std::make_shared<FlightProxy::Channel::ChannelT<FlightProxy::Core::MspPacket>>(
            std::weak_ptr(transport),
            encoder,
            decoder);

        tcp_channel->open();

        // Hacemos que el puntero de app_main "desaparezca".
        // La tarea del propio transport es duena de transport y por lo tanto se
        // autogesiona su cilco de vida
        transport.reset();

        while (true)
        {
            tcp_channel->sendPacket(FlightProxy::Core::MspPacket{
                .direction = '>',
                .command = 1,
                .payload = {0x01, 0x02, 0x03}});
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    else if (mode == TransportMode::TcpServer)
    {
        auto decoder_factory = []() -> std::shared_ptr<FlightProxy::Core::Protocol::IDecoderT<FlightProxy::Core::MspPacket>>
        {
            return std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();
        };
        auto encoder_factory = []() -> std::shared_ptr<FlightProxy::Core::Protocol::IEncoderT<FlightProxy::Core::MspPacket>>
        {
            return std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
        };

        auto tcp_server = std::make_shared<FlightProxy::Channel::ChannelServer<FlightProxy::Core::MspPacket>>(
            decoder_factory,
            encoder_factory);

        std::vector<std::shared_ptr<FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket>>> channels;

        tcp_server->onNewChannel = [&](std::shared_ptr<FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket>> channel)
        {
            FP_LOG_I("MAIN", "¡Nuevo canal TCP creado y abierto!");
            channels.push_back(channel);
            channel->onClose = [&, channel]()
            {
                FP_LOG_I("MAIN", "Canal TCP cerrado. Borrándolo de la lista.");
                channels.erase(std::remove(channels.begin(), channels.end(), channel), channels.end());
            };
            channel->onPacket = [&](const FlightProxy::Core::MspPacket &packet)
            {
                FP_LOG_I("MAIN", "Recibido MSP Packet: Command=%u, Payload Size=%zu", packet.command, packet.payload.size());
            };
        };

        tcp_server->start(12345);

        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    else if (mode == TransportMode::UdpServer)
    {
        auto transport = std::make_shared<FlightProxy::Transport::SimpleUDP>(12345);
        auto encoder = std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
        auto decoder = std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();
        auto udp_channel = std::make_shared<FlightProxy::Channel::ChannelT<FlightProxy::Core::MspPacket>>(
            std::weak_ptr(transport),
            encoder,
            decoder);

        udp_channel->onPacket = [&](const FlightProxy::Core::MspPacket &packet)
        {
            FP_LOG_I("MAIN", "Recibido MSP Packet en UDP: Command=%u, Payload Size=%zu", packet.command, packet.payload.size());
            FP_LOG_I("MAIN", "Contenido: %.*s", packet.payload.size(), packet.payload.data());
        };

        udp_channel->open();

        // Hacemos que el puntero de app_main "desaparezca".
        // La tarea del propio transport es duena de transport y por lo tanto se
        // autogesiona su cilco de vida
        transport.reset();

        while (true)
        {
            // para ver esto en terminal
            // dentor de pawershell ejecutas bash
            // corres ncat -u 10.26.145.236 12345 | hexdump -C
            // encias algo para que el udp coha tu ip y puerto
            udp_channel->sendPacket(FlightProxy::Core::MspPacket{
                .direction = '>',
                .command = 1,
                .payload = {0x01, 0x02, 0x03}});
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/*#include "FlightProxy/Core/Protocol/MspProtocol.h"

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

#include "FlightProxy/Core/Protocol/MspProtocol.h"
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