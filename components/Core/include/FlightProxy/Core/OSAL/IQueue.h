#pragma once
#include <cstddef>
#include <cstdint>

namespace FlightProxy {
namespace Core {
namespace OSAL {

// Esta clase NO la usa el usuario final. Es para los drivers.
class IQueue {
public:
  virtual ~IQueue() = default;

  // Mueve bytes crudos.
  // timeoutMs: 0 = no esperar, 0xFFFFFFFF = infinito
  virtual bool send(const void *itemBuffer, uint32_t timeoutMs) = 0;
  virtual bool receive(void *itemBuffer, uint32_t timeoutMs) = 0;

  virtual size_t size() const = 0;
};

} // namespace OSAL
} // namespace Core
} // namespace FlightProxy
