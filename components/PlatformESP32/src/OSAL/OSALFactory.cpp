#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include "FlightProxy/PlatformESP32/OSAL/FreeRTOSMutex.h"
#include "FlightProxy/PlatformESP32/OSAL/FreeRTOSQueue.h"
#include "FlightProxy/PlatformESP32/OSAL/FreeRTOSTask.h"

namespace FlightProxy {
namespace Core {
namespace OSAL {

// Implementación de createTask
std::unique_ptr<ITask>
OSALFactory::createTask(Core::OSAL::ITask::TaskFunction func,
                        const Core::OSAL::TaskConfig &config) {
  return std::make_unique<PlatformESP32::OSAL::FreeRTOSTask>(func, config);
}

// Implementación de createMutex
std::unique_ptr<IMutex> OSALFactory::createMutex() {
  return std::make_unique<PlatformESP32::OSAL::FreeRTOSMutex>();
}

std::unique_ptr<IQueue> OSALFactory::createRawQueue(size_t length,
                                                    size_t itemSize) {
  return std::make_unique<PlatformESP32::OSAL::FreeRTOSQueue>(itemSize, length);
}

// Implementación de sleep
void OSALFactory::sleep(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

// Implementación de getSystemTimeMs
uint64_t OSALFactory::getSystemTimeMs() {
  return static_cast<uint64_t>(xTaskGetTickCount()) * portTICK_PERIOD_MS;
}

} // namespace OSAL
} // namespace Core
} // namespace FlightProxy
