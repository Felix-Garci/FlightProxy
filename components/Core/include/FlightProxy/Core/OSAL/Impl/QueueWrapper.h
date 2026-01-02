#pragma once

#include "FlightProxy/Core/OSAL/IQueue.h"
#include "FlightProxy/Core/OSAL/IQueueT.h"

#include <memory>

namespace FlightProxy {
namespace Core {
namespace OSAL {

template <typename T> class QueueWrapper : public IQueueT<T> {
private:
  std::unique_ptr<IQueue> m_rawQueue;

public:
  explicit QueueWrapper(std::unique_ptr<IQueue> rawQueue)
      : m_rawQueue(std::move(rawQueue)) {}

  bool send(const T &item, uint32_t timeoutMs) override {
    if (!m_rawQueue)
      return false;
    return m_rawQueue->send(static_cast<const void *>(&item), timeoutMs);
  }

  bool receive(T &item, uint32_t timeoutMs) override {
    if (!m_rawQueue)
      return false;
    return m_rawQueue->receive(static_cast<void *>(&item), timeoutMs);
  }
};

} // namespace OSAL
} // namespace Core
} // namespace FlightProxy
