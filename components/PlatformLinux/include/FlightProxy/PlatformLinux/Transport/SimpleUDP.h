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

namespace FlightProxy {
namespace PlatformLinux {
namespace Transport {
class SimpleUDP : public FlightProxy::Core::Transport::ITransport,
                  public std::enable_shared_from_this<SimpleUDP> {
public:
  SimpleUDP(uint16_t port);
  ~SimpleUDP() override;

  void open() override;
  void close() override;
  void send(const uint8_t *data, size_t len) override;

private:
  int m_sock = -1;
  uint16_t m_port;

  // Estructuras estándar POSIX
  struct sockaddr_in m_last_sender_addr;
  socklen_t m_last_sender_len; // en Linux es socklen_t, no int
  bool m_has_last_sender = false;

  std::recursive_mutex mutex_;
  std::atomic<bool> isRunning_{false};

  void eventThreadFunc();
};
} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
