#include "FlightProxy/Core/Transport/TransportFactory.h"

#include "FlightProxy/PlatformESP32/Transport/ListenerTCP.h"
#include "FlightProxy/PlatformESP32/Transport/SimpleTCP.h"
#include "FlightProxy/PlatformESP32/Transport/SimpleUDP.h"
#include "FlightProxy/PlatformESP32/Transport/SimpleUart.h"

#include <memory>

namespace FlightProxy {
namespace Core {
namespace Transport {

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleUart(int port, int txpin, int rxpin,
                                   int baudrate) {
  return std::make_shared<PlatformESP32::Transport::SimpleUart>(
      static_cast<uart_port_t>(port), static_cast<gpio_num_t>(txpin),
      static_cast<gpio_num_t>(rxpin), static_cast<uint32_t>(baudrate));
}

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleUDP(uint16_t port) {
  return std::make_shared<PlatformESP32::Transport::SimpleUDP>(port);
}

std::shared_ptr<Core::Transport::ITransport>
TransportFactory::CreateSimpleTCP(const char *ip, uint16_t port) {
  return std::make_shared<PlatformESP32::Transport::SimpleTCP>(ip, port);
}

std::shared_ptr<Core::Transport::ITcpListener>
TransportFactory::CreateListenerTCP() {
  return std::make_shared<PlatformESP32::Transport::ListenerTCP>();
}

} // namespace Transport
} // namespace Core
} // namespace FlightProxy
