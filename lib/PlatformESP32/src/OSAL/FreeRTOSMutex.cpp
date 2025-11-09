#include "FlightProxy/PlatformESP32/OSAL/FreeRTOSMutex.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace OSAL
        {
            FreeRTOSMutex::FreeRTOSMutex()
            {
                // Usamos un mutex recursivo. Es más seguro para patrones
                // como MutexGuard si una función llama a otra función
                // que también bloquea el mismo mutex.
                mutexHandle_ = xSemaphoreCreateRecursiveMutex();
                // Aquí deberías añadir manejo de error (si mutexHandle_ es NULL)
            }

            FreeRTOSMutex::~FreeRTOSMutex()
            {
                vSemaphoreDelete(mutexHandle_);
            }

            void FreeRTOSMutex::lock()
            {
                // Bloquea indefinidamente
                xSemaphoreTakeRecursive(mutexHandle_, portMAX_DELAY);
            }

            void FreeRTOSMutex::unlock()
            {
                xSemaphoreGiveRecursive(mutexHandle_);
            }

            bool FreeRTOSMutex::tryLock(uint32_t timeout_ms)
            {
                // Convierte ms a ticks de FreeRTOS
                const TickType_t ticksToWait = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);

                BaseType_t result = xSemaphoreTakeRecursive(mutexHandle_, ticksToWait);

                // pdTRUE (1) si se obtuvo el mutex, pdFALSE (0) si no
                return (result == pdTRUE);
            }
        }
    }
}