#pragma once

#include "esp_log.h"
#include <cstdarg>

namespace FlightProxy
{
    namespace Utils
    {
        // Un envoltorio simple para esp_log
        class Logger
        {
        public:
            // Constructor: almacena el TAG
            Logger(const char *tag) : tag_(tag) {}

            // Métodos para cada nivel
            void info(const char *format, ...) const;
            void warn(const char *format, ...) const;
            void error(const char *format, ...) const;
            void debug(const char *format, ...) const;
            void verbose(const char *format, ...) const;

        private:
            const char *tag_;

            // Función helper genérica
            void log(esp_log_level_t level, const char *format, va_list args) const;
        };

    }
}