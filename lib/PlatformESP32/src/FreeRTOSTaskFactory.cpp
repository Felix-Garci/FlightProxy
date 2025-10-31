#include "FlightProxy/PlatformESP32/FreeRTOSTaskFactory.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {

        /**
         * @brief Adaptador estático.
         * FreeRTOS (que es C) no puede llamar a un método de un objeto C++
         * directamente. Necesita una función C estática.
         * Esta función actúa como "traductor".
         * * @param param El puntero void* que pasamos a xTaskCreate,
         * que será nuestro puntero al objeto IRunnable.
         */
        static void taskAdapter(void *param)
        {

            // 1. Convertir el puntero void* de nuevo a un puntero IRunnable
            Core::OSAL::IRunnable *runnable = static_cast<Core::OSAL::IRunnable *>(param);

            if (runnable)
            {
                // 2. Llamar al método run() del objeto
                runnable->run();
            }

            // 3. Si run() termina (no debería, si es un bucle),
            //    borramos la tarea para limpiar recursos.
            vTaskDelete(NULL);
        }

        // --- Implementación de la interfaz ITaskFactory ---

        void FreeRTOSTaskFactory::createTask(
            Core::OSAL::IRunnable &runnable,
            const char *taskName,
            uint32_t stackSize,
            uint32_t priority)
        {
            // Creamos la tarea de FreeRTOS
            xTaskCreate(
                taskAdapter, // La función C estática que definimos arriba
                taskName,
                stackSize,
                &runnable, // Pasamos el objeto IRunnable como parámetro
                priority,
                NULL // No guardamos el handle de la tarea
            );

            // TODO: Añadir manejo de error si xTaskCreate falla
        }

    } // namespace PlatformESP32
} // namespace FlightProxy