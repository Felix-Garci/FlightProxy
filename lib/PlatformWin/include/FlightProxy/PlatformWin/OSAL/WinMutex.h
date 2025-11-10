#pragma once
#include "FlightProxy/Core/OSAL/IMutex.h"
#include <mutex>
#include <chrono>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace OSAL
        {
            class WinMutex : public FlightProxy::Core::OSAL::IMutex
            {
            public:
                WinMutex() = default;
                virtual ~WinMutex() = default;

                void lock() override
                {
                    mutex_.lock();
                }

                void unlock() override
                {
                    mutex_.unlock();
                }

                bool tryLock(uint32_t timeout_ms) override
                {
                    if (timeout_ms == 0)
                    {
                        return mutex_.try_lock();
                    }
                    return mutex_.try_lock_for(std::chrono::milliseconds(timeout_ms));
                }

            private:
                // Usamos recursive_timed_mutex para igualar el comportamiento de FreeRTOS
                // que permite que el mismo hilo tome el mutex varias veces.
                std::recursive_timed_mutex mutex_;
            };
        }
    }
}