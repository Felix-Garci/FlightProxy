#include "FlightProxy/PlatformLinux/Utils/LinuxLogger.h"
#include <cstdarg>
#include <iostream>

namespace FlightProxy {
namespace PlatformLinux {
namespace Utils {
// Códigos de escape ANSI para colores en terminal Linux
static const char *ANSI_RESET = "\033[0m";
static const char *ANSI_RED = "\033[31m";
static const char *ANSI_YELLOW = "\033[33m";
static const char *ANSI_GREEN = "\033[32m";
static const char *ANSI_CYAN = "\033[36m";
static const char *ANSI_WHITE = "\033[37m";

void LinuxLogger::log(Core::Utils::LogLevel level, const char *tag,
                      const char *format, va_list args) {
  std::lock_guard<std::recursive_mutex> lock(logMutex_);

  const char *color = ANSI_WHITE;
  const char *prefix = " ";

  // Selección de prefijo y color
  switch (level) {
  case Core::Utils::LogLevel::Error:
    prefix = "E";
    color = ANSI_RED;
    break;
  case Core::Utils::LogLevel::Warn:
    prefix = "W";
    color = ANSI_YELLOW;
    break;
  case Core::Utils::LogLevel::Info:
    prefix = "I";
    color = ANSI_GREEN;
    break;
  case Core::Utils::LogLevel::Debug:
    prefix = "D";
    color = ANSI_CYAN;
    break;
  case Core::Utils::LogLevel::Verbose:
    prefix = "V";
    color = ANSI_WHITE; // O gris si tu terminal lo soporta ("\033[90m")
    break;
  }

  // Imprimir con color: [COLOR]Letra (Tag)[RESET] Mensaje
  std::cout << color << prefix << " (" << tag << ") " << ANSI_RESET;

  // vprintf imprime a stdout directamente
  vprintf(format, args);

  // Salto de línea final
  std::cout << std::endl;
}
} // namespace Utils
} // namespace PlatformLinux
} // namespace FlightProxy
