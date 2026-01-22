#include "FlightApplication.h"
// incluimos fabrica de osal
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include "FlightProxy/Core/Utils/Logger.h"

// incluimos fabrica de transportes
#include "FlightProxy/Core/Transport/TransportFactory.h"

// incluimos tipos
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/IbusProtocol.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"

// Almacen flexible

#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/AppLogic/AlmazenFlexibleID.h"

// Wifi
#if defined(ESP_PLATFORM)
#include "FlightProxy/Connectivity/WifiManager.h"
#endif

// Channels logic
#include "FlightProxy/Channel/ChannelAgregatorT.h"
#include "FlightProxy/Channel/ChannelDisgregatorT.h"
#include "FlightProxy/Channel/ChannelServer.h"
#include "FlightProxy/Channel/ChannelT.h"

// Hellpers para commands y para datanodes
#include "FlightProxy/AppLogic/AppSetupHelpers.h"

// App Logic - Command Manager
#include "FlightProxy/AppLogic/Command/AutoCommand.h"
#include "FlightProxy/AppLogic/Command/AutoCommandFactory.h"
#include "FlightProxy/AppLogic/Command/CommandManager.h"

// App Logic - Data Nodes
#include "FlightProxy/AppLogic/DataNode/DataNodesManagerT.h"

#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Emision_RC.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_Baro.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_IMU.h"
#include "FlightProxy/AppLogic/DataNode/DataNodes/Nodo_Recepcion_Status.h"

// App Logic - Controls
// #include "FlightProxy/AppLogic/Control/ControlManager.h"

// #include "FlightProxy/AppLogic/Control/Controls/CtrAltHold.h"
// #include "FlightProxy/AppLogic/Control/Controls/CtrPassThrow.h"

#include "FlightProxy/AppLogic/Control/ControlMaster.h"

FlightApplication::FlightApplication() {
  blackboard = std::make_shared<FlightProxy::AppLogic::AlmacenFlexible>();
  commandManager = std::make_shared<
      FlightProxy::AppLogic::Command::CommandManager<Packet>>();
  dataNodesManager =
      std::make_shared<FlightProxy::AppLogic::DataNode::DataNodesManager>();
  controlMaster_ =
      std::make_shared<FlightProxy::AppLogic::Control::ControlMaster>();
}

void FlightApplication::initialize() {
  setupNetwork();
  setupTCPchannel_in();
  setupUDPchannel_in();
  setupTCPchannel_out();

  setupCommandSystem();
  setupDataNodes();
  setupControlSystem();
}

void FlightApplication::setupNetwork() {
#if defined(ESP_PLATFORM)

  FlightProxy::Connectivity::WiFiManager wifiManager;

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

      this->droneIp = std::string(gatewat_srt);
    }
  } else {
    FP_LOG_E("MAIN", "Falló la conexión a WiFi.");
    // Manejar el fallo (quizás reiniciar el ESP)
  }
#endif
}

void FlightApplication::setupTCPchannel_in() {
  using namespace FlightProxy::AppLogic;
  using namespace FlightProxy::AppLogic::Command::Commands;
  using namespace FlightProxy::Core;

  auto decoder_factory = []() {
    return std::make_shared<Protocol::MspDecoder>();
  };
  auto encoder_factory = []() {
    return std::make_shared<Protocol::MspEncoder>();
  };
  auto listener_factory = []() {
    return Transport::Factory::CreateListenerTCP();
  };

  this->channelServerTCP_in =
      std::make_shared<FlightProxy::Channel::ChannelServer<Packet>>(
          decoder_factory, encoder_factory, listener_factory);

  this->channelAgregatorTCP_in =
      std::make_shared<FlightProxy::Channel::ChannelAgregatorT<Packet>>();

  this->channelServerTCP_in->onNewChannel =
      [this](std::shared_ptr<FlightProxy::Core::Channel::IChannelT<Packet>>
                 channel) {
        this->channelAgregatorTCP_in->addChannel(channel);
      };

  this->channelServerTCP_in->start(this->tcpPortInput);

  this->channelAgregatorTCP_in->onPacketFromAnyChannel =
      [this](const FlightProxy::Core::PacketEnvelope<Packet> &envelope) {
        return this->commandManager->enqueuePacket(envelope);
      };
  this->commandManager->responsehandler =
      [this](uint32_t channelId, std::unique_ptr<const Packet> packet) -> bool {
    this->channelAgregatorTCP_in->response(channelId, std::move(packet));
    return true;
  };
}

void FlightApplication::setupUDPchannel_in() {
  auto udp_transport = FlightProxy::Core::Transport::Factory::CreateSimpleUDP(
      this->udpPortInput);
  auto udp_transport_encoder =
      std::make_shared<FlightProxy::Core::Protocol::IbusEncoder>();
  auto udp_transport_decoder =
      std::make_shared<FlightProxy::Core::Protocol::IbusDecoder>();

  udp_transport->open();

  this->channelUDP_in = std::make_shared<FlightProxy::Channel::ChannelT<Bus>>(
      udp_transport, udp_transport_encoder, udp_transport_decoder);

  auto rcWriter =
      this->blackboard->registrarProductor<FlightProxy::Core::RCData>(
          FlightProxy::AppLogic::ID_RC_Input);
  this->channelUDP_in->onPacket =
      [rcWriter](std::unique_ptr<const Bus> packet) {
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
}

void FlightApplication::setupTCPchannel_out() {
  auto msp_transport = FlightProxy::Core::Transport::Factory::CreateSimpleTCP(
      this->droneIp.c_str(), this->dronePort);
  auto msp_transport_encoder =
      std::make_shared<FlightProxy::Core::Protocol::MspEncoder>();
  auto msp_transport_decoder =
      std::make_shared<FlightProxy::Core::Protocol::MspDecoder>();

  auto channelTCP_out =
      std::make_shared<FlightProxy::Channel::ChannelT<Packet>>(
          msp_transport, msp_transport_encoder, msp_transport_decoder);

  this->channelDisgregatorTCP_out =
      std::make_shared<FlightProxy::Channel::ChannelDisgregatorT<Packet>>(
          channelTCP_out,
          [](const Packet &pkt) -> FlightProxy::Channel::CommandId {
            return pkt.command;
          });
  channelTCP_out->open();
}

void FlightApplication::setupCommandSystem() {
  using namespace FlightProxy::AppLogic;
  using namespace FlightProxy::AppLogic::Command::Commands;
  using namespace FlightProxy::Core;

  auto cmdFactory = Command::AutoCommandFactory<Packet>(this->blackboard);

  cmdFactory.produceCMD<RCData>(ID_RC_Input, 1, 1);
  cmdFactory.produceCMD<RCData>(ID_RC_Output, 1, 0);
  cmdFactory.produceCMD<RCNORMData>(ID_RC_InputNorm, 1, 0);

  cmdFactory.produceCMD<BaroData>(ID_BARO_Data, 1, 0);
  // IMU
  // GPS

  cmdFactory.produceCMD<uint8_t>(ID_CTRL_LVL, 0, 1, "ctrlLevel");

  cmdFactory.produceCMD<PidCtrlIn>(ID_CTRL_LATVEL_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_LATVEL_OUT, 1, 0);

  cmdFactory.produceCMD<PidCtrlIn>(ID_CTRL_FRNTVEL_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_FRNTVEL_OUT, 1, 0);

  cmdFactory.produceCMD<PidCtrlVertVelIn>(ID_CTRL_VERTVEL_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_VERTVEL_OUT, 1, 0);

  cmdFactory.produceCMD<PidCtrlIn>(ID_CTRL_ANGVEL_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_ANGVEL_OUT, 1, 0);

  cmdFactory.produceCMD<PidCtrlIn>(ID_CTRL_HORPOS_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_HORPOS_OUT, 1, 0);

  cmdFactory.produceCMD<PidCtrlIn>(ID_CTRL_VERTPOS_IN, 0, 1);
  cmdFactory.produceCMD<PidCtrlOut>(ID_CTRL_VERTPOS_OUT, 1, 0);

  cmdFactory.loadCMDS(this->commandManager);

  /*
    Setup::registerProductorCommand<MSP_Set_InputRCData<Packet>, RCData,
    Packet>( this->commandManager, this->blackboard, ID_RC_Input);

    Setup::registerProductorCommand<MSP_Set_Hover<Packet>, uint16_t, Packet>(
        this->commandManager, this->blackboard, ID_HOVER);

    Setup::registerProductorCommand<MSP_Set_CtrActive<Packet>, std::string,
                                    Packet>(this->commandManager,
                                            this->blackboard, ID_ACTIVE_CTR);

    Setup::registerProductorCommand<MSP_Set_CtrSampPeriod<Packet>, uint64_t,
                                    Packet>(this->commandManager,
                                            this->blackboard,
    ID_SAMPPERIOID_CTR);

    Setup::registerProductorCommand<MSP_Set_PIDCts<Packet>, ControlPIDCts,
                                    Packet>(this->commandManager,
                                            this->blackboard, ID_PIDCST_CTR);

    Setup::registerConsumerCommand<MSP_Get_PIDVals<Packet>, ControlPIDVals,
                                   Packet>(this->commandManager,
    this->blackboard, ID_PIDVALS_CTR);
          */
}

void FlightApplication::setupDataNodes() {
  using namespace FlightProxy::AppLogic;
  using namespace FlightProxy::AppLogic::DataNode::DataNodes;
  using namespace FlightProxy::Core;

  Setup::addReceptionNode<Nodo_Recepcion_Status, StatusData, MspPacket>(
      this->dataNodesManager, this->channelDisgregatorTCP_out,
      Protocol::MSP_STATUS_DATA, this->blackboard, ID_STATUS_Data, 1000);

  Setup::addEmissionNode<Nodo_Emision_RC, RCData, MspPacket>(
      this->dataNodesManager, this->channelDisgregatorTCP_out,
      Protocol::MSP_RC_DATA, this->blackboard, ID_RC_Output, 10);

  Setup::addReceptionNode<Nodo_Recepcion_Baro, BaroData, MspPacket>(
      this->dataNodesManager, this->channelDisgregatorTCP_out,
      Protocol::MSP_BARO_DATA, this->blackboard, ID_BARO_Data, 100);

  Setup::addReceptionNode<Nodo_Recepcion_IMU, IMUData, MspPacket>(
      this->dataNodesManager, this->channelDisgregatorTCP_out,
      Protocol::MSP_IMU_DATA, this->blackboard, ID_IMU_Data, 500);
}

void FlightApplication::setupControlSystem() {

  this->controlMaster_->init(this->blackboard);
}

void FlightApplication::start() {
  commandManager->start();
  dataNodesManager->start();
  // controlManager->start();
  controlMaster_->start();
  while (true) {
    // FP_LOG_D(
    //     "MAIN", "RC recived %.2f",
    //     this->blackboard->getFrequency(FlightProxy::AppLogic::ID_RC_Input));
    FlightProxy::Core::OSAL::OSALFactory::sleep(1000);
  }
}
