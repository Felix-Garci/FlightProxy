#pragma once

#include <memory>
#include <string>

// Forward declarations para acelerar compilación
namespace FlightProxy {
namespace Core {
class MspPacket;
class IBUSPacket;
} // namespace Core
namespace AppLogic {
class AlmacenFlexible;
namespace Command {
template <typename T> class CommandManager;
template <typename T> class AutoCommandFactory;
namespace Commands {}
} // namespace Command
namespace DataNode {
class DataNodesManager;
}
namespace Control {
class ControlManager;
}
} // namespace AppLogic
namespace Channel {
template <typename T> class ChannelAgregatorT;
template <typename T> class ChannelDisgregatorT;
template <typename T> class ChannelServer;
template <typename T> class ChannelT;
} // namespace Channel
} // namespace FlightProxy

class FlightApplication {
public:
  using Packet = FlightProxy::Core::MspPacket;
  using Bus = FlightProxy::Core::IBUSPacket;

  FlightApplication();
  void initialize();
  void start();

private:
  // Métodos de configuración internos
  void setupNetwork();
  void setupTCPchannel_in();
  void setupUDPchannel_in();
  void setupTCPchannel_out();

  void setupCommandSystem();
  void setupDataNodes();
  void setupControlSystem();

  // Entrada tcp client (MSP msgs)
  int tcpPortInput = 12345;

  std::shared_ptr<FlightProxy::Channel::ChannelServer<Packet>>
      channelServerTCP_in;
  std::shared_ptr<FlightProxy::Channel::ChannelAgregatorT<Packet>>
      channelAgregatorTCP_in;
  std::shared_ptr<FlightProxy::AppLogic::Command::CommandManager<Packet>>
      commandManager;

  // Entrada udp client (IBus RC in)
  int udpPortInput = 12346;
  std::shared_ptr<FlightProxy::Channel::ChannelT<FlightProxy::Core::IBUSPacket>>
      channelUDP_in;

  // Salida tcp al dron ( MSP to dron)
  std::string droneIp = "127.0.0.1";
  // int dronePort = 15762;
  int dronePort = 5762;

  std::shared_ptr<FlightProxy::Channel::ChannelDisgregatorT<Packet>>
      channelDisgregatorTCP_out;
  std::shared_ptr<FlightProxy::AppLogic::DataNode::DataNodesManager>
      dataNodesManager;

  // Almacen
  std::shared_ptr<FlightProxy::AppLogic::AlmacenFlexible> blackboard;
  // Ctrl Mgr
  std::shared_ptr<FlightProxy::AppLogic::Control::ControlManager>
      controlManager;
};
