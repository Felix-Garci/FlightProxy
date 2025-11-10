#pragma once
#include <cstdint>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {

            class IMutex
            {
            public:
                virtual ~IMutex() = default;

                // --- Métodos de Instancia (lo que ya tenías) ---
                virtual void lock() = 0;
                virtual void unlock() = 0;
                virtual bool tryLock(uint32_t timeout_ms) = 0;
            };

        }
    }
}