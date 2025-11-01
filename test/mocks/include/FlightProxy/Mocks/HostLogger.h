#pragma once
#include "FlightProxy/Core/Utils/ILogger.h"
#include <cstdio>
#include <mutex>

namespace FlightProxy
{
    namespace Mocks
    {

        class HostLogger : public Core::Utils::ILogger
        {
        public:
            void log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args) override;

        private:
            // std::mutex para evitar logs corruptos si los tests son multihilo
            std::mutex logMutex_;
        };

    } // namespace Mocks
} // namespace FlightProxy