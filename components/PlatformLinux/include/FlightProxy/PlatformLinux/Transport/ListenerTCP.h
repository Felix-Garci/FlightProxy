#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
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
class ListenerTCP : public Core::Transport::ITcpListener,
                    public std::enable_shared_from_this<ListenerTCP> {
public:
  ListenerTCP();
  virtual ~ListenerTCP() override;

  bool startListening(uint16_t port) override;
  void stopListening() override;

private:
  void listenerThreadFunc();

  std::thread m_listener_thread;
  int m_server_sock = -1;
  std::recursive_mutex m_mutex;
  std::atomic<bool> m_is_running{false};
};

} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
