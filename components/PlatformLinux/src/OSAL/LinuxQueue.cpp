#include "FlightProxy/PlatformLinux/OSAL/LinuxQueue.h"
#include <chrono>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {

bool LinuxQueue::send(const void *itemBuffer, uint32_t timeoutMs) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Lógica simple de bloqueo si está llena (puedes mejorarla con wait_for)
  if (m_queue.size() >= m_maxItems) {
    return false; // O esperar según timeoutMs
  }

  // 1. Convertimos void* a bytes
  const uint8_t *bytes = static_cast<const uint8_t *>(itemBuffer);

  // 2. Creamos un vector y copiamos los datos (memcpy implícito en constructor
  // vector)
  std::vector<uint8_t> data(bytes, bytes + m_itemSize);

  // 3. Guardamos
  m_queue.push(data);

  lock.unlock();
  m_cond.notify_one();
  return true;
}

bool LinuxQueue::receive(void *itemBuffer, uint32_t timeoutMs) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Esperar datos
  if (m_queue.empty()) {
    if (m_cond.wait_for(lock, std::chrono::milliseconds(timeoutMs)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (m_queue.empty())
      return false; // Doble check
  }

  // 1. Sacamos el vector de bytes
  std::vector<uint8_t> data = m_queue.front();
  m_queue.pop();

  // 2. Copiamos los bytes a la memoria del usuario (memcpy)
  std::memcpy(itemBuffer, data.data(), m_itemSize);

  return true;
}

size_t LinuxQueue::size() const {
  // std::lock_guard ...
  return m_queue.size();
}

} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
