#include "FlightProxy/PlatformLinux/Transport/SimpleUDP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

namespace FlightProxy {
namespace PlatformLinux {
namespace Transport {
static const char *TAG = "SimpleUDP_Linux";

SimpleUDP::SimpleUDP(uint16_t port)
    : m_port(port), m_last_sender_len(sizeof(m_last_sender_addr)) {
  memset(&m_last_sender_addr, 0, sizeof(m_last_sender_addr));
}

SimpleUDP::~SimpleUDP() {
  close();
  FP_LOG_I(TAG, "Canal UDP destruido.");
}

void SimpleUDP::open() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (isRunning_.load()) {
    FP_LOG_W(TAG, "Hilo de eventos ya iniciado.");
    return;
  }

  if (m_sock == -1) {
    FP_LOG_I(TAG, "Intentando escuchar en UDP:%u...", m_port);

    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock < 0) {
      FP_LOG_E(TAG, "Error creando socket UDP: %s", strerror(errno));
      return;
    }

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(m_port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
      FP_LOG_E(TAG, "Error en bind UDP: %s", strerror(errno));
      ::close(m_sock);
      m_sock = -1;
      return;
    }
    FP_LOG_I(TAG, "Escuchando en UDP:%u! Socket fd: %d", m_port, m_sock);
  }

  isRunning_.store(true);
  try {
    std::thread([self = shared_from_this()]() {
      self->eventThreadFunc();
    }).detach();
  } catch (...) {
    isRunning_.store(false);
    ::close(m_sock);
    m_sock = -1;
    FP_LOG_E(TAG, "Error al crear hilo UDP.");
  }
}

void SimpleUDP::close() {
  int sock_to_close = -1;
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (m_sock != -1) {
      sock_to_close = m_sock;
    }
    isRunning_.store(false);
  }

  if (sock_to_close != -1) {
    // En Linux, cerrar el socket despierta al recvfrom con error
    shutdown(sock_to_close, SHUT_RDWR);
    ::close(sock_to_close);
  }
}

void SimpleUDP::send(const uint8_t *data, size_t len) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (m_sock == -1 || !m_has_last_sender || !data || len == 0)
    return;

  ssize_t sent =
      sendto(m_sock, (const char *)data, len, 0,
             (struct sockaddr *)&m_last_sender_addr, m_last_sender_len);

  if (sent < 0) {
    FP_LOG_E(TAG, "Error sendto UDP: %s", strerror(errno));
  }
}

void SimpleUDP::eventThreadFunc() {
  if (onOpen)
    onOpen();

  FP_LOG_I(TAG, "Hilo UDP iniciado.");
  std::vector<char> rx_buffer(1500); // MTU típica

  while (isRunning_.load()) {
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr); // socklen_t en Linux

    ssize_t len = recvfrom(m_sock, rx_buffer.data(), rx_buffer.size(), 0,
                           (struct sockaddr *)&sender_addr, &sender_len);

    if (len > 0) {
      {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        m_last_sender_addr = sender_addr;
        m_last_sender_len = sender_len;
        m_has_last_sender = true;
      }

      if (onData) {
        onData((uint8_t *)rx_buffer.data(), (size_t)len);
      }
    } else {
      if (isRunning_.load()) {
        FP_LOG_E(TAG, "Error recvfrom: %s", strerror(errno));
      }
      break;
    }
  }

  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (m_sock != -1) {
      ::close(m_sock);
      m_sock = -1;
    }
    m_has_last_sender = false;
    isRunning_.store(false);
  }

  if (onClose)
    onClose();
  FP_LOG_I(TAG, "Hilo UDP terminado.");
}
} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
