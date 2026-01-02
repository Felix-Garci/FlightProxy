#pragma once

#include "IQueueT.h"
#include "Impl/QueueWrapper.h"

#include "IMutex.h"
#include "ITask.h"

namespace FlightProxy {
namespace Core {
namespace OSAL {
class OSALFactory {
private:
  static std::unique_ptr<IQueue> createRawQueue(size_t queueLength,
                                                size_t itemSize);

public:
  static std::unique_ptr<ITask> createTask(ITask::TaskFunction func,
                                           const TaskConfig &config);

  static std::unique_ptr<IMutex> createMutex();

  template <typename T>
  static std::unique_ptr<IQueueT<T>> createQueue(size_t length) {
    auto rawQ = createRawQueue(length, sizeof(T));

    if (rawQ) {
      return std::make_unique<QueueWrapper<T>>(std::move(rawQ));
    }
    return nullptr;
  }

  static void sleep(uint32_t ms);

  static uint64_t getSystemTimeMs();
};
} // namespace OSAL
} // namespace Core
} // namespace FlightProxy
