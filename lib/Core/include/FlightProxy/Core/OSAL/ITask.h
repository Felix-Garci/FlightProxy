// FlightProxy/Core/OSAL/ITask.h
#pragma once
#include <string>
#include <functional>
#include <cstdint>

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {
            struct TaskConfig
            {
                std::string name = "Task";
                uint32_t stackSize = 4096; // Importante para FreeRTOS, ignorado en PC
                int priority = 5;          // Importante para FreeRTOS, a veces ignorado en PC
                int coreId = -1;           // Para ESP32 (pin to core), -1 = no pinned
            };

            class ITask
            {
            public:
                virtual ~ITask() = default;

                // Tipos comunes
                using TaskFunction = std::function<void()>;

                // Métodos básicos de control
                virtual void start() = 0;
                virtual void stop() = 0; // Solicita parar
                virtual void join() = 0; // Espera a que termine
                virtual bool isRunning() const = 0;
            };

        }
    }
}