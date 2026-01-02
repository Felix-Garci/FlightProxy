#include "FlightProxy/PlatformLinux/Transport/ListenerTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleTCP.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace FlightProxy {
namespace PlatformLinux {
namespace Transport {
static const char *TAG = "ListenerTCP_Linux";

ListenerTCP::ListenerTCP() {
  // Nada que inicializar globalmente
}

ListenerTCP::~ListenerTCP() {
  stopListening();

  if (m_listener_thread.joinable()) {
    m_listener_thread.join();
  }

  FP_LOG_I(TAG, "Listener destruido.");
}

bool ListenerTCP::startListening(uint16_t port) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  if (m_is_running.load()) {
    FP_LOG_W(TAG, "El listener ya estaba iniciado.");
    return true;
  }

  // 1. Crear socket
  m_server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_server_sock < 0) {
    FP_LOG_E(TAG, "Error creando socket: %s", strerror(errno));
    return false;
  }

  // --- LINUX SPECIFIC: Reuse Address ---
  // Esto es vital en Linux para evitar "Address already in use" si reinicias
  // rápido
  int opt = 1;
  if (setsockopt(m_server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    FP_LOG_W(TAG, "Fallo al poner SO_REUSEADDR: %s", strerror(errno));
  }

  // 2. Bind
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(m_server_sock, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    FP_LOG_E(TAG, "Error en bind: %s", strerror(errno));
    ::close(m_server_sock);
    m_server_sock = -1;
    return false;
  }

  // 3. Listen
  if (listen(m_server_sock, SOMAXCONN) < 0) {
    FP_LOG_E(TAG, "Error en listen: %s", strerror(errno));
    ::close(m_server_sock);
    m_server_sock = -1;
    return false;
  }

  // 4. Iniciar hilo
  m_is_running.store(true);
  try {
    m_listener_thread = std::thread(&ListenerTCP::listenerThreadFunc, this);
  } catch (...) {
    m_is_running.store(false);
    ::close(m_server_sock);
    m_server_sock = -1;
    FP_LOG_E(TAG, "Error al crear el hilo del listener");
    return false;
  }

  FP_LOG_I(TAG, "Listener iniciado en puerto %d", port);
  return true;
}

void ListenerTCP::stopListening() {
  int sock_to_close = -1;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_is_running.load())
      return;

    m_is_running.store(false);
    sock_to_close = m_server_sock;
  }

  if (sock_to_close != -1) {
    // Forzar cierre para desbloquear accept
    shutdown(sock_to_close, SHUT_RDWR);
    ::close(sock_to_close);
  }
}

void ListenerTCP::listenerThreadFunc() {
  FP_LOG_I(TAG, "Hilo de Listener iniciado.");

  while (m_is_running.load()) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // accept() es bloqueante
    int client_sock =
        accept(m_server_sock, (struct sockaddr *)&client_addr, &addr_len);

    if (client_sock < 0) {
      if (m_is_running.load()) {
        FP_LOG_E(TAG, "Error en accept: %s", strerror(errno));
      } else {
        FP_LOG_I(TAG, "Listener detenido voluntariamente.");
      }
      break;
    }

    FP_LOG_I(TAG, "Cliente conectado! Socket fd: %d", client_sock);

    // Importante: SimpleTCP toma ownership del file descriptor
    auto new_transport = std::make_shared<SimpleTCP>(client_sock);

    if (onNewTransport) {
      onNewTransport(new_transport);
    }
  }

  // Limpieza final
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_server_sock != -1) {
      // Aseguramos que se marca como cerrado
      // Aunque normalmente ya lo hemos cerrado en stopListening
      m_server_sock = -1;
    }
    m_is_running.store(false);
  }
  FP_LOG_I(TAG, "Hilo de Listener terminado.");
}

} // namespace Transport
} // namespace PlatformLinux
} // namespace FlightProxy
