#pragma once

// 1. Incluimos la Interfaz que vamos a implementar
#include "FlightProxy/Core/OSAL/IQueue.h"

// 2. Incluimos las dependencias REALES de la plataforma
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {

        // 3. Declaramos la clase que IMPLEMENTA la interfaz
        template <typename T>
        class FreeRTOSQueue : public Core::OSAL::IQueue<T>
        {
        public:
            /**
             * @brief Crea una cola de FreeRTOS.
             * @param queueLength El número máximo de ítems que puede contener.
             */
            FreeRTOSQueue(uint32_t queueLength)
            {
                queueHandle_ = xQueueCreate(queueLength, sizeof(T));

                // Aquí deberías añadir manejo de error (ej. ESP_ERROR_CHECK)
                // si queueHandle_ es NULL (no hay memoria).
            }

            virtual ~FreeRTOSQueue()
            {
                vQueueDelete(queueHandle_);
            }

            // --- Implementación de la interfaz IQueue ---

            bool send(const T &item, uint32_t timeout_ms) override
            {
                const TickType_t ticksToWait = pdMS_TO_TICKS(timeout_ms);

                // Usamos &item para pasar la dirección del ítem a la cola
                BaseType_t result = xQueueSend(queueHandle_, &item, ticksToWait);

                return (result == pdTRUE);
            }

            bool receive(T &item, uint32_t timeout_ms) override
            {
                const TickType_t ticksToWait = pdMS_TO_TICKS(timeout_ms);

                // Usamos &item para pasar la dirección donde se copiará el ítem
                BaseType_t result = xQueueReceive(queueHandle_, &item, ticksToWait);

                return (result == pdTRUE);
            }

        private:
            QueueHandle_t queueHandle_;
        };

    } // namespace PlatformESP32
} // namespace FlightProxy