#include "FlightProxy/PlatformESP32/Utils/EspLogger.h"
#include <string>
namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Utils
        {
            void EspLogger::log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args)
            {
                esp_log_level_t espLevel = toEspLogLevel(level);

                // 1. Creamos un nuevo string de formato que incluya el TAG explícitamente.
                // Puedes ajustar el formato "[%s] " a tu gusto (ej. añadir timestamp si quieres).
                std::string new_format = "";

                // Opcional: Añadir color según nivel (ESP-IDF lo hace con códigos ANSI)
                // if (espLevel == ESP_LOG_ERROR) new_format += "\033[0;31m"; // Rojo para error

                new_format += "[";
                new_format += tag;
                new_format += "] ";
                new_format += format;
                new_format += "\n"; // Añadimos salto de línea al final

                // Opcional: Resetear color
                // if (espLevel == ESP_LOG_ERROR) new_format += "\033[0m";

                // 2. Usamos la función 'v' con el nuevo formato que YA incluye el TAG visualmente.
                // Seguimos pasando 'tag' como segundo argumento para que el filtrado de logs de ESP-IDF siga funcionando.
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
        }
    } // namespace PlatformESP32
} // namespace FlightProxy