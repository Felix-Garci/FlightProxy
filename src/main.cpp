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
#include "FlightProxy/Core/Protocol/IbusProtocol.h"

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
#include "FlightProxy/Channel/ChannelDisgregatorT.h"

// App Logic - Command Manager
#include "FlightProxy/AppLogic/Command/CommandManager.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_BasicRead_Command.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_ReadRCblackboard.h"

// App Logic - Data Nodes
#include "FlightProxy/AppLogic/DataNode/DataNodesManagerT.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_IMU.h"

void app()
{
    // Looger init
    FlightProxy::Core::Utils::Logger::setInstance(logger);

    FP_LOG_I("main", "Logger inicializado.");

    // Almacen flexible init
    enum DataIDs : FlightProxy::AppLogic::DataID
    {
        ID_RC_Input = 1,
        ID_IMU_Data = 10,
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

    //________________________________________COMAND FLUX___________________________________________________________

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
    };

    tcp_server->start(12345);

    // Command Manager
    auto commandManager = std::make_shared<FlightProxy::AppLogic::Command::CommandManager<Packet>>();

    // Conectamos agregator con command manager
    // Paquetes de ida
    agregadorTcpClients->onPacketFromAnyChannel = [commandManager](const FlightProxy::Core::PacketEnvelope<Packet> &envelope)
    {
        return commandManager->enqueuePacket(envelope);
    };
    // Paquetes de vuelta
    commandManager->responsehandler = [agregadorTcpClients](uint32_t channelId, std::unique_ptr<const Packet> packet) -> bool
    {
        agregadorTcpClients->response(channelId, std::move(packet));
        return true;
    };

    // Registrar los comandos
    auto commans1 = std::make_shared<FlightProxy::AppLogic::Command::Commands::MSP_BasicRead_Command<Packet>>();
    commandManager->registerCommand(commans1);

    auto commans2 = std::make_shared<FlightProxy::AppLogic::Command::Commands::MSP_ReadRCblackboard<Packet>>(
        blackboard->registrarConsumidor<FlightProxy::Core::IBUSPacket::ChannelsT>(ID_RC_Input));
    commandManager->registerCommand(commans2);

    // commans1.reset();

    commandManager->start();

    //________________________________________RC FLUX___________________________________________________________

    using Bus = FlightProxy::Core::IBUSPacket;

    // Servidor UDP
    auto udp_transport = FlightProxy::Core::Transport::Factory::CreateSimpleUDP(12346);
    auto udp_transport_encoder = std::make_shared<FlightProxy::Core::Protocol::IbusEncoder>();
    auto udp_transport_decoder = std::make_shared<FlightProxy::Core::Protocol::IbusDecoder>();

    auto udp_server = std::make_shared<FlightProxy::Channel::ChannelT<Bus>>(udp_transport, udp_transport_encoder, udp_transport_decoder);

    std::function<void(Bus::ChannelsT)> rcWriter = blackboard->registrarProductor<Bus::ChannelsT>(ID_RC_Input);

    udp_server->onPacket = [rcWriter](std::unique_ptr<const Bus> packet)
    {
        rcWriter(packet->channels);
        return;
    };

    udp_server->open();

    // Limpiamos referencia para que solo quede dentro del tasl del udp
    udp_transport.reset();

    //________________________________________MSP to dron___________________________________________________________
    // Cliente TCP hacia el dron
    auto msp_transport = FlightProxy::Core::Transport::Factory::CreateSimpleTCP("127.0.0.1", 5760);
    auto msp_transport_encoder = std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
    auto msp_transport_decoder = std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();

    auto msp_client = std::make_shared<FlightProxy::Channel::ChannelT<Packet>>(msp_transport, msp_transport_encoder, msp_transport_decoder);

    auto msp_client_channel = std::make_shared<FlightProxy::Channel::ChannelDisgregatorT<Packet>>(msp_client,
                                                                                                  [](const Packet &pkt) -> FlightProxy::Channel::CommandId
                                                                                                  {
                                                                                                      return pkt.command;
                                                                                                  });
    msp_client->open();
    //________________________________________Data Nodes___________________________________________________________

    auto dataNodesManager = std::make_shared<FlightProxy::AppLogic::DataNode::DataNodesManager>();

    auto nodoRecepcionIMU = std::make_shared<FlightProxy::AppLogic::DataNode::DataNodes::Nodo_Recepcion_IMU>(
        msp_client_channel->createVirtualChannel(FlightProxy::Core::Protocol::MSP_IMU_DATA),
        blackboard->registrarProductor<FlightProxy::Core::IMUData>(ID_IMU_Data));

    dataNodesManager->addDataNode(nodoRecepcionIMU, 50); // cada 100 ms

    dataNodesManager->start();
    //________________________________________Bucle infinito___________________________________________________________

    auto getimu = blackboard->registrarConsumidor<FlightProxy::Core::IMUData>(ID_IMU_Data);

    while (true)
    {
        auto imu = getimu(); // actualiza el dato interno
        FP_LOG_I("MAIN", "IMU Data: Accel[%d, %d, %d] Gyro[%d, %d, %d] frecuency: %.2f Hz",
                 imu.accel_x, imu.accel_y, imu.accel_z,
                 imu.gyro_x, imu.gyro_y, imu.gyro_z,
                 blackboard->getFrequency(ID_IMU_Data));

        FlightProxy::Core::OSAL::Factory::sleep(1000);
    }
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
