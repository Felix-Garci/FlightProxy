#pragma once

// Detectar plataforma (puedes usar defines de tu sistema de build, ej. CMake)
#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/PlatformFactory.h"
// Alias: "OSALFactory" será "PlatformESP32::PlatformFactory" aquí
namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {
            using Factory = FlightProxy::PlatformESP32::PlatformFactory;
        }
    }
}
#else
// Asumimos PC si no es ESP32 (o añades más #elif para otras plataformas)
#include "FlightProxy/PlatformPC/PlatformFactory.h"
namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {
            using Factory = FlightProxy::PlatformPC::PlatformFactory;
        }
    }
}
#endif