#pragma once

#include "FlightProxy/Core/OSAL/ITask.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <atomic>

namespace FlightProxy
{
    namespace PlatformESP32
    {
        class FreeRTOSTask : public Core::OSAL::ITask
        {
        public:
            FreeRTOSTask(TaskFunction func, const Core::OSAL::TaskConfig &config)
                : m_func(func), m_config(config)
            {
                // Creamos el semáforo para join() y lo tomamos inmediatamente.
                // Cuando la tarea termine, hará "give" y liberará a quien llame a join().
                m_joinSem = xSemaphoreCreateBinary();
            }

            virtual ~FreeRTOSTask()
            {
                // Aseguramos que la tarea se detenga si destruimos el objeto
                if (isRunning())
                {
                    stop();
                }
                if (m_joinSem)
                {
                    vSemaphoreDelete(m_joinSem);
                }
            }

            void start() override
            {
                if (isRunning())
                {
                    // FP_LOG_W("FreeRTOSTask", "Tarea %s ya está corriendo", m_config.name.c_str());
                    return;
                }

                // Determinar afinidad de núcleo
                BaseType_t coreId = tskNO_AFFINITY;
                if (m_config.coreId >= 0 && m_config.coreId < portNUM_PROCESSORS)
                {
                    coreId = m_config.coreId;
                }

                m_isTaskRunning = true;

                // Reset del semáforo de join por si se reutiliza el objeto
                xSemaphoreTake(m_joinSem, 0);

                BaseType_t res = xTaskCreatePinnedToCore(
                    taskWrapper,           // Función estática
                    m_config.name.c_str(), // Nombre
                    m_config.stackSize,    // Tamaño de stack en bytes (o palabras, depende del port, en ESP32 son bytes)
                    this,                  // Parámetro: pasamos 'this' para acceder a miembros
                    m_config.priority,     // Prioridad
                    &m_handle,             // Handle para guardar
                    coreId);               // Núcleo

                if (res != pdPASS)
                {
                    m_isTaskRunning = false;
                    // FP_LOG_E("FreeRTOSTask", "Fallo al crear tarea %s", m_config.name.c_str());
                }
            }

            void stop() override
            {
                if (isRunning() && m_handle)
                {
                    // NOTA: vTaskDelete es una forma brusca de parar.
                    // Lo ideal sería tener un flag que tu m_func revise periódicamente.
                    // Para esta implementación genérica, forzamos la detención.
                    vTaskDelete(m_handle);
                    m_handle = nullptr;
                    m_isTaskRunning = false;
                    // Liberamos el join por si alguien estaba esperando
                    xSemaphoreGive(m_joinSem);
                }
            }

            void join() override
            {
                // Si la tarea está corriendo, esperamos a que termine (tome el semáforo)
                if (m_joinSem)
                {
                    xSemaphoreTake(m_joinSem, portMAX_DELAY);
                    // Lo devolvemos inmediatamente por si otros hilos también hacen join()
                    xSemaphoreGive(m_joinSem);
                }
            }

            bool isRunning() const override
            {
                return m_isTaskRunning.load();
            }

        private:
            // Wrapper estático que se ajusta a la firma que espera FreeRTOS
            static void taskWrapper(void *pvParameters)
            {
                FreeRTOSTask *self = static_cast<FreeRTOSTask *>(pvParameters);

                // 1. Ejecuta la función del usuario
                if (self->m_func)
                {
                    self->m_func();
                }

                // 2. Marcar como finalizada
                self->m_isTaskRunning = false;
                self->m_handle = nullptr;

                // 3. Señalizar a cualquiera que esté haciendo join()
                if (self->m_joinSem)
                {
                    xSemaphoreGive(self->m_joinSem);
                }

                // 4. Auto-eliminación de la tarea FreeRTOS
                vTaskDelete(NULL);
            }

            TaskFunction m_func;
            Core::OSAL::TaskConfig m_config;
            TaskHandle_t m_handle = nullptr;
            SemaphoreHandle_t m_joinSem = nullptr;
            std::atomic<bool> m_isTaskRunning{false};
        };
    } // namespace PlatformESP32
} // namespace FlightProxy