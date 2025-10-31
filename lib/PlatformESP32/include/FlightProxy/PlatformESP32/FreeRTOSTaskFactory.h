#pragma once

// 1. Incluimos la Interfaz
#include "FlightProxy/Core/OSAL/ITaskFactory.h"

// 2. Incluimos las dependencias REALES
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {

        // 3. Declaramos la clase de implementación
        class FreeRTOSTaskFactory : public Core::OSAL::ITaskFactory
        {
        public:
            void createTask(
                Core::OSAL::IRunnable &runnable,
                const char *taskName,
                uint32_t stackSize,
                uint32_t priority) override;
        };

    } // namespace PlatformESP32
} // namespace FlightProxy