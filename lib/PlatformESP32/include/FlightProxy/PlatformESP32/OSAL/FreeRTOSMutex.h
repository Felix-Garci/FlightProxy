#pragma once
#include "FlightProxy/Core/OSAL/IMutex.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace OSAL
        {
            class FreeRTOSMutex : public Core::OSAL::IMutex
            {
            public:
                FreeRTOSMutex() { mutexHandle_ = xSemaphoreCreateRecursiveMutex(); }
                virtual ~FreeRTOSMutex() { vSemaphoreDelete(mutexHandle_); }

                void lock() override { xSemaphoreTakeRecursive(mutexHandle_, portMAX_DELAY); }
                void unlock() override { xSemaphoreGiveRecursive(mutexHandle_); }
                bool tryLock(uint32_t timeout_ms) override
                {
                    // Convierte ms a ticks de FreeRTOS
                    const TickType_t ticksToWait = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);

                    BaseType_t result = xSemaphoreTakeRecursive(mutexHandle_, ticksToWait);

                    // pdTRUE (1) si se obtuvo el mutex, pdFALSE (0) si no
                    return (result == pdTRUE);
                }

            private:
                SemaphoreHandle_t mutexHandle_;
            };
        }
    }
}