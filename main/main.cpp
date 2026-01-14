#include "FlightApplication.h"

#include "FlightProxy/Core/Utils/Logger.h"
#if defined(ESP_PLATFORM)
#include "FlightProxy/PlatformESP32/Utils/EspLogger.h"
static FlightProxy::PlatformESP32::Utils::EspLogger logger;
#else
#include "FlightProxy/PlatformLinux/Utils/LinuxLogger.h"
static FlightProxy::PlatformLinux::Utils::LinuxLogger logger;
#endif

#include "FlightProxy/Core/OSAL/OSALFactory.h"

void app() {
  FlightProxy::Core::Utils::Logger::setInstance(logger);
  auto app = FlightApplication();

  app.initialize();
  app.start();

  while (true) {
    FlightProxy::Core::OSAL::OSALFactory::sleep(1000);
  }
}

#if defined(ESP_PLATFORM)
extern "C" void app_main(void) { app(); }
#else
int main() {
  app();
  return 0;
}
#endif

//
