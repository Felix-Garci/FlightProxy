#pragma once
#include "FlightProxy/Core/Utils/ILogger.h"
#include "esp_log.h" // ¡Esta librería SÍ puede incluir esp_log.h!

namespace FlightProxy
{
    namespace PlatformESP32
    {

        class EspLogger : public Core::Utils::ILogger
        {
        public:
            void log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args) override;

        private:
            // Mapea nuestro LogLevel al de esp-idf
            esp_log_level_t toEspLogLevel(Core::Utils::LogLevel level);
        };

    } // namespace PlatformESP32
} // namespace FlightProxy