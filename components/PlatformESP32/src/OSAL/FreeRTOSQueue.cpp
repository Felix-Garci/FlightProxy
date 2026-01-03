#include "FlightProxy/PlatformESP32/OSAL/FreeRTOSQueue.h"

namespace FlightProxy {
namespace PlatformESP32 {
namespace OSAL {
FreeRTOSQueue::FreeRTOSQueue(uint32_t itemSize, uint32_t queueLength) {
  queueHandle_ = xQueueCreate(queueLength, itemSize);
}

FreeRTOSQueue::~FreeRTOSQueue() {
  if (queueHandle_ != nullptr)
    vQueueDelete(queueHandle_);
}

bool FreeRTOSQueue::send(const void *itemBuffer, uint32_t timeoutMs) {
  if (queueHandle_ == nullptr)
    return false;

  TickType_t ticks;
  if (timeoutMs == 0) {
    ticks = 0;
  } else if (timeoutMs == 0xFFFFFFFF) { // Tu constante de infinito
    ticks = portMAX_DELAY;
  } else {
    ticks = pdMS_TO_TICKS(timeoutMs);
  }

  // xQueueSend copia los bytes desde itemBuffer hacia la cola interna
  BaseType_t res = xQueueSend(queueHandle_, itemBuffer, ticks);
  return (res == pdTRUE);
}

bool FreeRTOSQueue::receive(void *itemBuffer, uint32_t timeoutMs) {
  if (queueHandle_ == nullptr)
    return false;

  TickType_t ticks;
  if (timeoutMs == 0) {
    ticks = 0;
  } else if (timeoutMs == 0xFFFFFFFF) {
    ticks = portMAX_DELAY;
  } else {
    ticks = pdMS_TO_TICKS(timeoutMs);
  }

  // xQueueReceive copia los bytes desde la cola hacia itemBuffer
  BaseType_t res = xQueueReceive(queueHandle_, itemBuffer, ticks);
  return (res == pdTRUE);
}

size_t FreeRTOSQueue::size() const {
  if (queueHandle_ == nullptr)
    return 0;
  return (size_t)uxQueueMessagesWaiting(queueHandle_);
}

} // namespace OSAL
} // namespace PlatformESP32
} // namespace FlightProxy
