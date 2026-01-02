#pragma once

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
#endif
