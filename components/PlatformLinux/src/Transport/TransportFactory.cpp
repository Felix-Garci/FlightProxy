#include "FlightProxy/Core/Transport/TransportFactory.h"

#include "FlightProxy/PlatformLinux/Transport/ListenerTCP.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleTCP.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleUDP.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleUart.h"

#include <memory>
#include <string>

namespace FlightProxy {
namespace Core {
namespace Transport {

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleUart(int port, int txpin, int rxpin,
                                   int baudrate) {
  return std::make_shared<PlatformLinux::Transport::SimpleUart>(
      std::to_string(port), static_cast<uint32_t>(baudrate));
}

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleUDP(uint16_t port) {
  return std::make_shared<PlatformLinux::Transport::SimpleUDP>(port);
}

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleTCP(const char *ip, uint16_t port) {
  return std::make_shared<PlatformLinux::Transport::SimpleTCP>(ip, port);
}

std::shared_ptr<Core::Transport::ITcpListener>
TransportFactory::CreateListenerTCP() {
  return std::make_shared<PlatformLinux::Transport::ListenerTCP>();
}
} // namespace Transport
} // namespace Core
} // namespace FlightProxy
