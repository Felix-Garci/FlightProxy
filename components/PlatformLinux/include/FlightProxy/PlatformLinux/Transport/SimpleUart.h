#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace FlightProxy {
namespace PlatformLinux {
namespace Transport {
/**
 * @brief Versión MOCK de SimpleUART para Linux.
 * Igual que en Windows, actúa como loopback.
 */
class SimpleUart : public FlightProxy::Core::Transport::ITransport {
public:
  SimpleUart(const std::string &portName, uint32_t baudRate);
  virtual ~SimpleUart() = default;

  void open() override;
  void close() override;
  void send(const uint8_t *data, size_t len) override;

  // --- Miembros para inspección en Tests ---
  bool isOpen = false;
  std::vector<uint8_t> lastSentData;
  int sendCount = 0;

private:
  std::recursive_mutex mutex_;
};
} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
