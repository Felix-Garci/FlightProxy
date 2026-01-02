#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include "FlightProxy/PlatformLinux/OSAL/LinuxMutex.h"
#include "FlightProxy/PlatformLinux/OSAL/LinuxQueue.h"
#include "FlightProxy/PlatformLinux/OSAL/LinuxTask.h"

namespace FlightProxy {
namespace Core {
namespace OSAL {

// Implementación de createTask
std::unique_ptr<ITask>
OSALFactory::createTask(Core::OSAL::ITask::TaskFunction func,
                        const Core::OSAL::TaskConfig &config) {
  return std::make_unique<PlatformLinux::OSAL::LinuxTask>(func, config);
}

// Implementación de createMutex
std::unique_ptr<IMutex> OSALFactory::createMutex() {
  return std::make_unique<PlatformLinux::OSAL::LinuxMutex>();
}

std::unique_ptr<IQueue> OSALFactory::createRawQueue(size_t length,
                                                    size_t itemSize) {
  return std::make_unique<PlatformLinux::OSAL::LinuxQueue>(length, itemSize);
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

} // namespace OSAL
} // namespace Core
} // namespace FlightProxy
