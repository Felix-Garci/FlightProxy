#pragma once
#include "FlightProxy/Core/Utils/ILogger.h"
#include <cstdio>
#include <mutex>

namespace FlightProxy {
namespace PlatformLinux {
namespace Utils {
class LinuxLogger : public Core::Utils::ILogger {
public:
  void log(Core::Utils::LogLevel level, const char *tag, const char *format,
           va_list args) override;

private:
  // std::recursive_mutex es ideal para loggers para evitar deadlocks
  // si una función dentro del lock intenta loguear de nuevo.
  std::recursive_mutex logMutex_;
};
} // namespace Utils
} // namespace PlatformLinux
} // namespace FlightProxy
