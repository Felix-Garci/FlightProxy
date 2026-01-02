#pragma once
#include "FlightProxy/Core/OSAL/IQueue.h"
#include <condition_variable>
#include <cstring> // para memcpy
#include <mutex>
#include <queue>
#include <vector>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {

class LinuxQueue : public FlightProxy::Core::OSAL::IQueue {
private:
  // Como es void*, guardamos vectores de bytes (std::vector<uint8_t>)
  std::queue<std::vector<uint8_t>> m_queue;

  std::mutex m_mutex;
  std::condition_variable m_cond;

  size_t m_maxItems;
  size_t m_itemSize; // Recordamos de qué tamaño son los bloques

public:
  LinuxQueue(size_t length, size_t itemSize)
      : m_maxItems(length), m_itemSize(itemSize) {}

  bool send(const void *itemBuffer, uint32_t timeoutMs) override;
  bool receive(void *itemBuffer, uint32_t timeoutMs) override;
  size_t size() const override;
};

} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
