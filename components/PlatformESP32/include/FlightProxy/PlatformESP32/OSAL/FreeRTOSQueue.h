#pragma once

#include "FlightProxy/Core/OSAL/IQueue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstdint>

namespace FlightProxy {
namespace PlatformESP32 {
namespace OSAL {
class FreeRTOSQueue : public Core::OSAL::IQueue {
public:
  FreeRTOSQueue(uint32_t itemSize, uint32_t queueLength);

  virtual ~FreeRTOSQueue();

  bool send(const void *itemBuffer, uint32_t timeoutMs) override;

  bool receive(void *itemBuffer, uint32_t timeoutMs) override;

  size_t size() const override;

private:
  QueueHandle_t queueHandle_;
};
} // namespace OSAL
} // namespace PlatformESP32
} // namespace FlightProxy
