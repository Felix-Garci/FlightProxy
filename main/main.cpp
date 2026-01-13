#include "FlightProxy/Core/Utils/Logger.h"

#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/Utils/EspLogger.h"
static FlightProxy::PlatformESP32::Utils::EspLogger logger;
#else
#include "FlightProxy/PlatformLinux/Utils/LinuxLogger.h"
static FlightProxy::PlatformLinux::Utils::LinuxLogger logger;
#endif

// incluimos fabrica de osal
#include "FlightProxy/Core/OSAL/OSALFactory.h"

// incluimos fabrica de transportes
#include "FlightProxy/Core/Transport/TransportFactory.h"

// incluimos tipos
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/IbusProtocol.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"

// Almacen flexible
#include "FlightProxy/AppLogic/AlmacenFlexible.h"

// Wifi
#if defined(ESP_PLATFORM)
#include "FlightProxy/Connectivity/WifiManager.h"
#endif

// Channels logic
#include "FlightProxy/Channel/ChannelAgregatorT.h"
#include "FlightProxy/Channel/ChannelDisgregatorT.h"
#include "FlightProxy/Channel/ChannelServer.h"
#include "FlightProxy/Channel/ChannelT.h"

// App Logic - Command Manager
#include "FlightProxy/AppLogic/Command/CommandManager.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Get_DinamicsTelemetry.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Get_PIDVals.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Set_CtrActive.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Set_CtrSampPeriod.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Set_Hover.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Set_InputRCData.h"
#include "FlightProxy/AppLogic/Command/Commands/MSP_Set_PIDCts.h"

// App Logic - Data Nodes
#include "FlightProxy/AppLogic/DataNode/DataNodesManagerT.h"

#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Emision_RC.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_Baro.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_IMU.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_Status.h"

// App Logic - Controls
#include "FlightProxy/AppLogic/Control/ControlManager.h"

#include "FlightProxy/AppLogic/Control/Controls/CtrAltHold.h"
#include "FlightProxy/AppLogic/Control/Controls/CtrPassThrow.h"

void app() {
  // Looger init
  FlightProxy::Core::Utils::Logger::setInstance(logger);

  FP_LOG_I("main", "Logger inicializado.");

  // Almacen flexible init
  enum DataIDs : FlightProxy::AppLogic::DataID {
    ID_STATUS_Data = 0,
    ID_RC_Input = 1,
    ID_RC_Output = 2,
    ID_IMU_Data = 10,
    ID_BARO_Data = 11,
    ID_ACTIVE_CTR = 100,
    ID_SAMPPERIOID_CTR = 101,
    ID_PIDVALS_CTR = 102,
    ID_PIDCST_CTR = 103,
    ID_HOVER = 104,
  };

  auto blackboard = std::make_shared<FlightProxy::AppLogic::AlmacenFlexible>();

  // Network
  // UDP input
  uint16_t UDP_PORT_INPUT = 12346;
  // TCP input
  uint16_t TCP_PORT_INPUT = 12345;
  // TCP output
  std::string TCP_IP_OUTPUT = "127.0.0.1";
  uint16_t TCP_PORT_OUTPUT = 15762;

// WIFI
#if defined(ESP_PLATFORM)
  bool forze_ip = true;

  FlightProxy::Connectivity::WiFiManager wifiManager;
  if (forze_ip) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
      esp_netif_dhcpc_stop(netif);
      esp_netif_ip_info_t ip_info;
      IP4_ADDR(&ip_info.ip, 10, 42, 0, 100); // La IP del ESP32
      IP4_ADDR(&ip_info.gw, 10, 42, 0, 1);   // La IP del PC (Gateway)
      IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

      esp_netif_set_ip_info(netif, &ip_info);

      FP_LOG_I("MAIN", "IP Estática configurada: 10.42.0.100");
    }
  }

  FP_LOG_I("MAIN", "Intentando conectar a WiFi...");
  bool connected = wifiManager.connect("ESP32_Dev_Net", "esp32pass");

  if (connected) {
    FP_LOG_I("MAIN", "¡WiFi conectado exitosamente!");
    // Aquí puedes iniciar otras tareas que dependan de la red
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey(
        "WIFI_STA_DEF"); // Obtiene el netif de la estación

    if (netif) {
      esp_netif_get_ip_info(netif, &ip_info);
      FP_LOG_I("MAIN", "--- DEBUG DE RED ---");
      FP_LOG_I("MAIN", "Mi IP:      " IPSTR, IP2STR(&ip_info.ip));
      FP_LOG_I("MAIN", "Mi Netmask: " IPSTR, IP2STR(&ip_info.netmask));
      FP_LOG_I("MAIN", "Mi Gateway: " IPSTR, IP2STR(&ip_info.gw));
      FP_LOG_I("MAIN", "----------------------");

      char gatewat_srt[16];
      esp_ip4addr_ntoa(&ip_info.gw, gatewat_srt, sizeof(gatewat_srt));

      TCP_IP_OUTPUT = std::string(gatewat_srt);
    }
  } else {
    FP_LOG_E("MAIN", "Falló la conexión a WiFi.");
    // Manejar el fallo (quizás reiniciar el ESP)
  }
#endif

  // Definicion de paquete a usar
  using Packet = FlightProxy::Core::MspPacket;

  //________________________COMAND FLUX_______________________

  // Servidor TCP
  auto decoder_factory =
      []() -> std::shared_ptr<FlightProxy::Core::Protocol::IDecoderT<Packet>> {
    return std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();
  };
  auto encoder_factory =
      []() -> std::shared_ptr<FlightProxy::Core::Protocol::IEncoderT<Packet>> {
    return std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
  };
  auto listener_factory =
      []() -> std::shared_ptr<FlightProxy::Core::Transport::ITcpListener> {
    return FlightProxy::Core::Transport::Factory::CreateListenerTCP();
  };
  auto tcp_server =
      std::make_shared<FlightProxy::Channel::ChannelServer<Packet>>(
          decoder_factory, encoder_factory, listener_factory);

  auto agregadorTcpClients =
      std::make_shared<FlightProxy::Channel::ChannelAgregatorT<Packet>>();

  tcp_server->onNewChannel =
      [agregadorTcpClients](
          std::shared_ptr<FlightProxy::Core::Channel::IChannelT<Packet>>
              channel) { agregadorTcpClients->addChannel(channel); };

  tcp_server->start(TCP_PORT_INPUT);

  // Command Manager
  auto commandManager = std::make_shared<
      FlightProxy::AppLogic::Command::CommandManager<Packet>>();

  // Conectamos agregator con command manager
  // Paquetes de ida
  agregadorTcpClients->onPacketFromAnyChannel =
      [commandManager](
          const FlightProxy::Core::PacketEnvelope<Packet> &envelope) {
        return commandManager->enqueuePacket(envelope);
      };
  // Paquetes de vuelta
  commandManager->responsehandler =
      [agregadorTcpClients](uint32_t channelId,
                            std::unique_ptr<const Packet> packet) -> bool {
    agregadorTcpClients->response(channelId, std::move(packet));
    return true;
  };

  // Registrar los comandos
  auto setRCInput =
      blackboard->registrarProductor<FlightProxy::Core::RCData>(ID_RC_Input);
  auto cmdRCInput = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Set_InputRCData<Packet>>(
      setRCInput);
  commandManager->registerCommand(cmdRCInput);

  auto setHover = blackboard->registrarProductor<uint16_t>(ID_HOVER);
  auto cmdHover = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Set_Hover<Packet>>(
      setHover);
  commandManager->registerCommand(cmdHover);

  auto setActiveCtr =
      blackboard->registrarProductor<std::string>(ID_ACTIVE_CTR);
  setActiveCtr("passThrow");
  auto cmdActiveControl = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Set_CtrActive<Packet>>(
      setActiveCtr);
  commandManager->registerCommand(cmdActiveControl);

  auto setSampMs = blackboard->registrarProductor<uint64_t>(ID_SAMPPERIOID_CTR);
  setSampMs(100);
  auto cmdSampMsControl = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Set_CtrSampPeriod<Packet>>(
      setSampMs);
  commandManager->registerCommand(cmdSampMsControl);

  auto getPIDVals =
      blackboard->registrarConsumidor<FlightProxy::Core::ControlPIDVals>(
          ID_PIDVALS_CTR);
  auto cmdPIDVals = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Get_PIDVals<Packet>>(
      getPIDVals);
  commandManager->registerCommand(cmdPIDVals);

  auto setPIDCst =
      blackboard->registrarProductor<FlightProxy::Core::ControlPIDCts>(
          ID_PIDCST_CTR);
  auto cmdPIDCst = std::make_shared<
      FlightProxy::AppLogic::Command::Commands::MSP_Set_PIDCts<Packet>>(
      setPIDCst);
  commandManager->registerCommand(cmdPIDCst);

  auto rcGetter =
      blackboard->registrarConsumidor<FlightProxy::Core::RCData>(ID_RC_Input);
  auto baroGetter =
      blackboard->registrarConsumidor<FlightProxy::Core::BaroData>(
          ID_BARO_Data);
  auto cmdDinamicsTel =
      std::make_shared<FlightProxy::AppLogic::Command::Commands::
                           MSP_Get_DinamicsTelemetry<Packet>>(rcGetter,
                                                              baroGetter);
  commandManager->registerCommand(cmdDinamicsTel);

  commandManager->start();

  //________________________RC FLUX_________________________
  using Bus = FlightProxy::Core::IBUSPacket;

  // Servidor UDP
  auto udp_transport =
      FlightProxy::Core::Transport::Factory::CreateSimpleUDP(UDP_PORT_INPUT);
  auto udp_transport_encoder =
      std::make_shared<FlightProxy::Core::Protocol::IbusEncoder>();
  auto udp_transport_decoder =
      std::make_shared<FlightProxy::Core::Protocol::IbusDecoder>();

  auto udp_server = std::make_shared<FlightProxy::Channel::ChannelT<Bus>>(
      udp_transport, udp_transport_encoder, udp_transport_decoder);

  auto rcWriter =
      blackboard->registrarProductor<FlightProxy::Core::RCData>(ID_RC_Input);

  udp_server->onPacket = [rcWriter](std::unique_ptr<const Bus> packet) {
    FlightProxy::Core::RCData rcData;
    rcData.roll = packet->channels[0];
    rcData.pitch = packet->channels[1];
    rcData.throttle = packet->channels[2];
    rcData.yaw = packet->channels[3];
    rcData.aux1 = packet->channels[4];
    rcData.aux2 = packet->channels[5];

    for (size_t i = 0; i < 8; ++i) {
      rcData.aux_channels[i] = packet->channels[6 + i];
    }

    rcWriter(rcData);
    return;
  };

  udp_server->open();
  // Limpiamos referencia para que solo quede dentro del tasl del udp
  udp_transport.reset();

  //___________MSP to dron_______________

  // Cliente TCP hacia el dron
  auto msp_transport = FlightProxy::Core::Transport::Factory::CreateSimpleTCP(
      TCP_IP_OUTPUT.c_str(), TCP_PORT_OUTPUT);
  auto msp_transport_encoder =
      std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
  auto msp_transport_decoder =
      std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();

  auto msp_client = std::make_shared<FlightProxy::Channel::ChannelT<Packet>>(
      msp_transport, msp_transport_encoder, msp_transport_decoder);

  auto msp_client_channel =
      std::make_shared<FlightProxy::Channel::ChannelDisgregatorT<Packet>>(
          msp_client, [](const Packet &pkt) -> FlightProxy::Channel::CommandId {
            return pkt.command;
          });
  msp_client->open();

  //__________________Data Nodes____________________________

  auto dataNodesManager =
      std::make_shared<FlightProxy::AppLogic::DataNode::DataNodesManager>();

  auto nodoRecepcionIMU = std::make_shared<
      FlightProxy::AppLogic::DataNode::DataNodes::Nodo_Recepcion_IMU>(
      msp_client_channel->createVirtualChannel(
          FlightProxy::Core::Protocol::MSP_IMU_DATA),
      blackboard->registrarProductor<FlightProxy::Core::IMUData>(ID_IMU_Data));
  dataNodesManager->addDataNode(nodoRecepcionIMU, 500);

  auto nodoRecepcionStatus = std::make_shared<
      FlightProxy::AppLogic::DataNode::DataNodes::Nodo_Recepcion_Status>(
      msp_client_channel->createVirtualChannel(
          FlightProxy::Core::Protocol::MSP_STATUS_DATA),
      blackboard->registrarProductor<FlightProxy::Core::StatusData>(
          ID_STATUS_Data));
  dataNodesManager->addDataNode(nodoRecepcionStatus, 1000);

  auto nodoEmisionRC = std::make_shared<
      FlightProxy::AppLogic::DataNode::DataNodes::Nodo_Emision_RC>(
      msp_client_channel->createVirtualChannel(
          FlightProxy::Core::Protocol::MSP_RC_DATA),
      blackboard->registrarConsumidor<FlightProxy::Core::RCData>(ID_RC_Output));
  dataNodesManager->addDataNode(nodoEmisionRC, 50);

  auto nodoRecepcionBaro = std::make_shared<
      FlightProxy::AppLogic::DataNode::DataNodes::Nodo_Recepcion_Baro>(
      msp_client_channel->createVirtualChannel(
          FlightProxy::Core::Protocol::MSP_BARO_DATA),
      blackboard->registrarProductor<FlightProxy::Core::BaroData>(
          ID_BARO_Data));
  dataNodesManager->addDataNode(nodoRecepcionBaro, 100);
  dataNodesManager->start();

  // __________________Control ______________________________

  auto activeControlGetter =
      blackboard->registrarConsumidor<std::string>(ID_ACTIVE_CTR);

  auto samplingPeriodMsGetter =
      blackboard->registrarConsumidor<uint64_t>(ID_SAMPPERIOID_CTR);

  auto ctrManager =
      std::make_shared<FlightProxy::AppLogic::Control::ControlManager>(
          activeControlGetter, samplingPeriodMsGetter);

  // passThrow
  auto getrc =
      blackboard->registrarConsumidor<FlightProxy::Core::RCData>(ID_RC_Input);
  auto setrc =
      blackboard->registrarProductor<FlightProxy::Core::RCData>(ID_RC_Output);

  auto passThrow =
      std::make_unique<FlightProxy::AppLogic::Control::Controls::CtrPassThrow>(
          getrc, setrc);

  ctrManager->addControl("passThrow", std::move(passThrow));

  // altHold
  auto getbaro = blackboard->registrarConsumidor<FlightProxy::Core::BaroData>(
      ID_BARO_Data);
  auto getPIDCTS =
      blackboard->registrarConsumidor<FlightProxy::Core::ControlPIDCts>(
          ID_PIDCST_CTR);

  auto setPIDVals =
      blackboard->registrarProductor<FlightProxy::Core::ControlPIDVals>(
          ID_PIDVALS_CTR);
  auto getHover = blackboard->registrarConsumidor<uint16_t>(ID_HOVER);

  auto altHold =
      std::make_unique<FlightProxy::AppLogic::Control::Controls::CtrAltHold>(
          getrc, getbaro, setrc, setPIDVals, getPIDCTS, getHover);

  ctrManager->addControl("altHold", std::move(altHold));

  ctrManager->start();

  //_________Bucle infinito______________________

  while (true) {

    // FP_LOG_D("MAIN", "frecuencia %.2g Hz",
    //         blackboard->getFrequency(ID_RC_Output));
    FlightProxy::Core::OSAL::OSALFactory::sleep(1000);
  }
}

#if defined(ESP_PLATFORM)
extern "C" void app_main(void) { app(); }
#else
int main() {
  app();
  return 0;
}
#endif

//
