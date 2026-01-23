#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
#include "FlightProxy/Core/Transport/ITransport.h"

#include <memory>

namespace FlightProxy {
namespace Core {
namespace Transport {

class TransportFactory {
public:
  static std::shared_ptr<Core::Transport::ITransport>
  CreateSimpleUart(int port, int txpin, int rxpin, int baudrate);

  static std::shared_ptr<Core::Transport::ITransport>
  CreateSimpleUDP(uint16_t port);

  static std::shared_ptr<Core::Transport::ITransport>
  CreateSimpleTCP(const char *ip, uint16_t port);

  static std::shared_ptr<Core::Transport::ITcpListener> CreateListenerTCP();
};

} // namespace Transport
} // namespace Core
} // namespace FlightProxy
/*
#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/Transport/TransportFactory.h"

namespace FlightProxy {
namespace Core {
namespace Transport {
using Factory = FlightProxy::PlatformESP32::Transport::TransportFactory;
}
} // namespace Core
} // namespace FlightProxy
#else
// Asumimos PC si no es ESP32 (o añades más #elif para otras plataformas)
#include "FlightProxy/PlatformLinux/Transport/TransportFactory.h"

namespace FlightProxy {
namespace Core {
namespace Transport {
using Factory = FlightProxy::PlatformLinux::Transport::TransportFactory;
}
} // namespace Core
} // namespace FlightProxy
#endif */
