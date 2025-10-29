#include "Logger.h"

namespace FlightProxy
{
    namespace Utils
    {

        // Función "helper" privada
        void Logger::log(esp_log_level_t level, const char *format, va_list args) const
        {
            // esp_log_vwrite es la función subyacente que usan las macros
            // La 'v' es porque acepta una 'va_list'
            esp_log_writev(level, tag_, format, args);
        }

        // Implementación de cada nivel
        void Logger::info(const char *format, ...) const
        {
            va_list args;
            va_start(args, format);
            log(ESP_LOG_INFO, format, args);
            va_end(args);
        }

        void Logger::warn(const char *format, ...) const
        {
            va_list args;
            va_start(args, format);
            log(ESP_LOG_WARN, format, args);
            va_end(args);
        }

        void Logger::error(const char *format, ...) const
        {
            va_list args;
            va_start(args, format);
            log(ESP_LOG_ERROR, format, args);
            va_end(args);
        }

        void Logger::debug(const char *format, ...) const
        {
            va_list args;
            va_start(args, format);
            log(ESP_LOG_DEBUG, format, args);
            va_end(args);
        }

        void Logger::verbose(const char *format, ...) const
        {
            va_list args;
            va_start(args, format);
            log(ESP_LOG_VERBOSE, format, args);
            va_end(args);
        }
    }
}