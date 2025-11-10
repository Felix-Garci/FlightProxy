#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/PlatformWin/Utils/WinLogger.h"
#include <iostream>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Utils
        {

            void HostLogger::log(Core::Utils::LogLevel level, const char *tag, const char *format, va_list args)
            {
                std::lock_guard lock(logMutex_);

                // Imprime el Nivel y Tag
                switch (level)
                {
                case Core::Utils::LogLevel::Error:
                    std::cout << "E";
                    break;
                case Core::Utils::LogLevel::Warn:
                    std::cout << "W";
                    break;
                case Core::Utils::LogLevel::Info:
                    std::cout << "I";
                    break;
                case Core::Utils::LogLevel::Debug:
                    std::cout << "D";
                    break;
                case Core::Utils::LogLevel::Verbose:
                    std::cout << "V";
                    break;
                }
                std::cout << " (" << tag << ") ";

                // Imprime el mensaje formateado
                vprintf(format, args);
                std::cout << std::endl; // Añade salto de línea
            }
        } // namespace Utils
    } // namespace Mocks
} // namespace FlightProxy