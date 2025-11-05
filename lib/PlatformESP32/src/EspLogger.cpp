#include "FlightProxy/PlatformESP32/EspLogger.h"
#include <string>
namespace FlightProxy
{
    namespace PlatformESP32
    {

        void EspLogger::log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args)
        {
            esp_log_level_t espLevel = toEspLogLevel(level);

            // Usamos la función 'v' (variadic) de esp_log
            // esp_log_writev(espLevel, tag, format, args);

            // 1. Creamos un nuevo string de formato que envuelve al original
            std::string new_format = "\n";
            new_format += format;
            new_format += "\n";
            // 2. Usamos la función 'v' (variadic) de esp_log con el *nuevo* formato
            esp_log_writev(espLevel, tag, new_format.c_str(), args);
        }

        esp_log_level_t EspLogger::toEspLogLevel(Core::Utils::LogLevel level)
        {
            switch (level)
            {
            case Core::Utils::LogLevel::Error:
                return ESP_LOG_ERROR;
            case Core::Utils::LogLevel::Warn:
                return ESP_LOG_WARN;
            case Core::Utils::LogLevel::Info:
                return ESP_LOG_INFO;
            case Core::Utils::LogLevel::Debug:
                return ESP_LOG_DEBUG;
            case Core::Utils::LogLevel::Verbose:
                return ESP_LOG_VERBOSE;
            default:
                return ESP_LOG_INFO;
            }
        }

    } // namespace PlatformESP32
} // namespace FlightProxy