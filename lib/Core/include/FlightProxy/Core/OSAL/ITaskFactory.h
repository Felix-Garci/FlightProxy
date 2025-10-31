#pragma once
#include "FlightProxy/Core/OSAL/IRunnable.h"
#include <cstdint>

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {

            class ITaskFactory
            {
            public:
                virtual ~ITaskFactory() = default;

                /**
                 * @brief Crea e inicia una nueva tarea del sistema operativo.
                 * * @param runnable El objeto cuyo método run() será ejecutado.
                 * @param taskName Nombre descriptivo para la tarea (debugging).
                 * @param stackSize Tamaño de la pila en bytes.
                 * @param priority Prioridad de la tarea (ej. 5).
                 */
                virtual void createTask(
                    IRunnable &runnable,
                    const char *taskName,
                    uint32_t stackSize,
                    uint32_t priority) = 0;
            };

        } // namespace OSAL
    } // namespace Core
} // namespace FlightProxy