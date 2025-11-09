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
                FreeRTOSMutex();
                virtual ~FreeRTOSMutex();

                void lock() override;
                void unlock() override;
                bool tryLock(uint32_t timeout_ms) override;

            private:
                SemaphoreHandle_t mutexHandle_;
            };
        }
    }
}