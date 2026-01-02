#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include <arpa/inet.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace FlightProxy {
namespace PlatformLinux {
namespace Transport {
class SimpleTCP : public FlightProxy::Core::Transport::ITransport,
                  public std::enable_shared_from_this<SimpleTCP> {
public:
  // Constructor para socket aceptado (servidor)
  SimpleTCP(int accepted_socket);
  // Constructor para cliente
  SimpleTCP(const char *ip, uint16_t port);
  ~SimpleTCP() override;

  void open() override;
  void close() override;
  void send(const uint8_t *data, size_t len) override;

private:
  int m_sock = -1; // -1 es el equivalente a INVALID_SOCKET en Linux
  uint16_t port_ = 0;
  char ip_[16] = {0};

  std::recursive_mutex mutex_;
  std::atomic<bool> isRunning_{false};

  void eventThreadFunc();
};
} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
