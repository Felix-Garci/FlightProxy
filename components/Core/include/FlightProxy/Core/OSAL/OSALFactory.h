#pragma once

// Detectar plataforma (puedes usar defines de tu sistema de build, ej. CMake)
#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/OSAL/OSALFactory.h"

namespace FlightProxy {
namespace Core {
namespace OSAL {
using Factory = FlightProxy::PlatformESP32::OSAL::OSALFactory;
}
} // namespace Core
} // namespace FlightProxy
#else
// Asumimos PC si no es ESP32 (o añades más #elif para otras plataformas)
#include "FlightProxy/PlatformLinux/OSAL/OSALFactory.h"
namespace FlightProxy {
namespace Core {
namespace OSAL {
using Factory = FlightProxy::PlatformLinux::OSAL::OSALFactory;
}
} // namespace Core
} // namespace FlightProxy
#endif
