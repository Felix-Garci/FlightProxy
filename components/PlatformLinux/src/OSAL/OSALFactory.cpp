#include "FlightProxy/PlatformLinux/OSAL/OSALFactory.h"

#include "FlightProxy/PlatformLinux/OSAL/LinuxMutex.h"
#include "FlightProxy/PlatformLinux/OSAL/LinuxTask.h"

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {

// Implementación de createTask
std::unique_ptr<Core::OSAL::ITask>
OSALFactory::createTask(Core::OSAL::ITask::TaskFunction func,
                        const Core::OSAL::TaskConfig &config) {
  return std::make_unique<OSAL::LinuxTask>(func, config);
}

// Implementación de createMutex
std::unique_ptr<Core::OSAL::IMutex> OSALFactory::createMutex() {
  return std::make_unique<OSAL::LinuxMutex>();
}

// Implementación de sleep
void OSALFactory::sleep(uint32_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Implementación de getSystemTimeMs
uint64_t OSALFactory::getSystemTimeMs() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

// LA TEMPLATE NO VA AQUÍ

} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
