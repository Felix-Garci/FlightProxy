#pragma once
#include <cstdint>

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
                virtual void lock() = 0;
                virtual void unlock() = 0;
                virtual bool tryLock(uint32_t timeout_ms) = 0;
            };

        }
    }
}