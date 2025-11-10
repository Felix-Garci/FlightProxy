#include "FlightProxy/Core/Utils/Logger.h"

#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/Utils/EspLogger.h"
static FlightProxy::PlatformESP32::Utils::EspLogger logger;
#else
#include "FlightProxy/PlatformWin/Utils/WinLogger.h"
static FlightProxy::PlatformWin::Utils::HostLogger logger;
#endif

// incluimos fabrica de osal
#include "FlightProxy/Core/OSAL/OSALFactory.h"

// incluimos fabrica de transportes
#include "FlightProxy/Core/Transport/TransportFactory.h"

// incluimos tipos
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"

// Almacen flexible
#include "FlightProxy/AppLogic/AlmacenFlexible.h"

// Wifi
#if defined(ESP_PLATFORM)
#include "FlightProxy/Connectivity/WifiManager.h"
#endif

// Channels logic
#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Channel/ChannelServer.h"
#include "FlightProxy/Channel/ChannelAgregatorT.h"

// App
#include "FlightProxy/AppLogic/Command/CommandManager.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_BasicRead_Command.h"

void app()
{
    // Looger init
    FlightProxy::Core::Utils::Logger::setInstance(logger);

    FP_LOG_I("main", "Logger inicializado.");

    // Almacen flexible init
    enum DataIDs : FlightProxy::AppLogic::DataID
    {
        ID_Dato_Prueva_Int,
    };

    auto blackboard = std::make_shared<FlightProxy::AppLogic::AlmacenFlexible>();

// WIFI
#if defined(ESP_PLATFORM)
    FlightProxy::Connectivity::WiFiManager wifiManager;

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
#endif

    // Definicion de paquete a usar
    using Packet = FlightProxy::Core::MspPacket;

    // Servidor TCP
    auto decoder_factory = []() -> std::shared_ptr<FlightProxy::Core::Protocol::IDecoderT<Packet>>
    {
        return std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();
    };
    auto encoder_factory = []() -> std::shared_ptr<FlightProxy::Core::Protocol::IEncoderT<Packet>>
    {
        return std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
    };
    auto listener_factory = []() -> std::shared_ptr<FlightProxy::Core::Transport::ITcpListener>
    {
        return FlightProxy::Core::Transport::Factory::CreateListenerTCP();
    };
    auto tcp_server = std::make_shared<FlightProxy::Channel::ChannelServer<Packet>>(decoder_factory, encoder_factory, listener_factory);

    auto agregadorTcpClients = std::make_shared<FlightProxy::Channel::ChannelAgregatorT<Packet>>();

    tcp_server->onNewChannel = [agregadorTcpClients](std::shared_ptr<FlightProxy::Core::Channel::IChannelT<Packet>> channel)
    {
        agregadorTcpClients->addChannel(channel);
        FP_LOG_W("AGREG", "Agregador nuevo chanel");
    };

    tcp_server->start(12345);

    // Command Manager
    auto commandManager = std::make_shared<FlightProxy::AppLogic::Command::CommandManager<Packet>>();

    // Conectamos agregator con command manager
    // Paquetes de ida
    agregadorTcpClients->onPacketFromAnyChannel = [commandManager](const FlightProxy::Core::PacketEnvelope<Packet> &envelope)
    {
        FP_LOG_W("AGREG", "New oaquet throow agreg");
        return commandManager->enqueuePacket(envelope);
    };
    // Paquetes de vuelta
    commandManager->responsehandler = [agregadorTcpClients](uint32_t channelId, std::unique_ptr<const Packet> packet) -> bool
    {
        FP_LOG_W("CMDMGR", "Sending response back through agregator");
        agregadorTcpClients->response(channelId, std::move(packet));
        return true;
    };

    // Registrar los comandos
    auto commans1 = std::make_shared<FlightProxy::AppLogic::Command::Commands::MSP_BasicRead_Command<Packet>>();
    commandManager->registerCommand(commans1);

    // commans1.reset();

    commandManager->start();

    while (true)
        FlightProxy::Core::OSAL::Factory::sleep(1000);
}

#if defined(ESP_PLATFORM)
extern "C" void app_main(void)
{
    app();
}
#else
int main()
{
    app();
    return 0;
}
#endif

//
