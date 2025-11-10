#pragma once
#include "FlightProxy/Core/Utils/ILogger.h"
#include <cstdio>
#include <mutex>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Utils
        {

            class HostLogger : public Core::Utils::ILogger
            {
            public:
                void log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args) override;

            private:
                // std::mutex para evitar logs corruptos si los tests son multihilo
                std::recursive_mutex logMutex_;
            };
        } // namespace Utils
    } // namespace PlatformWin
} // namespace FlightProxy