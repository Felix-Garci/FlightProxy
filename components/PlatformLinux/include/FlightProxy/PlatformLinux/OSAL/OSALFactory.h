#pragma once
#include "LinuxMutex.h"
#include "LinuxQueue.h"
#include "LinuxTask.h"
#include <chrono>
#include <memory>
#include <thread>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {

struct OSALFactory {
  // 1. Declaraciones (sin cuerpo, poner ';')
  static std::unique_ptr<Core::OSAL::ITask>
  createTask(Core::OSAL::ITask::TaskFunction func,
             const Core::OSAL::TaskConfig &config);

  // 2. TEMPLATE: Se TIENE que quedar aquí completo
  template <typename T>
  static std::unique_ptr<Core::OSAL::IQueue<T>>
  createQueue(uint32_t queueLength) {
    return std::make_unique<OSAL::LinuxQueue<T>>(queueLength);
  }

  // 3. Declaraciones
  static std::unique_ptr<Core::OSAL::IMutex> createMutex();
  static void sleep(uint32_t ms);
  static uint64_t getSystemTimeMs();
};

} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
