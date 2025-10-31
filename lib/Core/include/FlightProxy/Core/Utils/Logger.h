#pragma once

#include "FlightProxy/Core/Utils/ILogger.h" // Incluye la interfaz
#include <cstdarg>                          // Para va_start, va_end

namespace FlightProxy
{
    namespace Core
    {
        namespace Utils
        {

            /**
             * @brief Singleton de Logging.
             * Mantiene una instancia estática de la implementación de ILogger.
             * Las macros FP_LOG_... llamarán a esta clase.
             */
            class Logger
            {
            public:
                /**
                 * @brief Inyecta la implementación de logging (desde main.cpp o tests).
                 */
                static void setInstance(ILogger &instance)
                {
                    loggerInstance_ = &instance;
                }

                /**
                 * @brief Función de log pública que las macros usarán.
                 */
                static void log(LogLevel level, const char *tag, const char *format, ...)
                {
                    // Si nadie ha inyectado un logger, no hacemos nada.
                    if (!loggerInstance_)
                        return;

                    va_list args;
                    va_start(args, format);
                    loggerInstance_->log(level, tag, format, args);
                    va_end(args);
                }

            private:
                static ILogger *loggerInstance_;
            };

        } // namespace Utils
    } // namespace Core
} // namespace FlightProxy

// --- Tus Macros, ahora refactorizadas ---
// (Necesitamos una implementación .cpp para inicializar el puntero)
#define FP_LOG_E(tag, format, ...) FlightProxy::Core::Utils::Logger::log(FlightProxy::Core::Utils::LogLevel::Error, tag, format, ##__VA_ARGS__)
#define FP_LOG_W(tag, format, ...) FlightProxy::Core::Utils::Logger::log(FlightProxy::Core::Utils::LogLevel::Warn, tag, format, ##__VA_ARGS__)
#define FP_LOG_I(tag, format, ...) FlightProxy::Core::Utils::Logger::log(FlightProxy::Core::Utils::LogLevel::Info, tag, format, ##__VA_ARGS__)
#define FP_LOG_D(tag, format, ...) FlightProxy::Core::Utils::Logger::log(FlightProxy::Core::Utils::LogLevel::Debug, tag, format, ##__VA_ARGS__)
#define FP_LOG_V(tag, format, ...) FlightProxy::Core::Utils::Logger::log(FlightProxy::Core::Utils::LogLevel::Verbose, tag, format, ##__VA_ARGS__)