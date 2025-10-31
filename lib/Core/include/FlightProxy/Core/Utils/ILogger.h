#pragma once
#include <cstdarg>

namespace FlightProxy
{
    namespace Core
    {
        namespace Utils
        {

            enum class LogLevel
            {
                Error,
                Warn,
                Info,
                Debug,
                Verbose
            };

            class ILogger
            {
            public:
                virtual ~ILogger() = default;

                /**
                 * @brief Función de log de bajo nivel que las implementaciones deben proveer.
                 */
                virtual void log(LogLevel level, const char *tag, const char *format, va_list args) = 0;
            };

        } // namespace Utils
    } // namespace Core
} // namespace FlightProxy