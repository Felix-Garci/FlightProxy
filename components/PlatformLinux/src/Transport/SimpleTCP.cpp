#include "FlightProxy/PlatformLinux/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>
#include <netdb.h>       // getaddrinfo
#include <unistd.h>      // close
#include <sys/socket.h>

namespace FlightProxy
{
    namespace PlatformLinux
    {
        namespace Transport
        {
            static const char *TAG = "SimpleTCP_Linux";

            // En Linux no necesitamos initWinsock ni cleanupWinsock

            SimpleTCP::SimpleTCP(int accepted_socket)
                : m_sock(accepted_socket), port_(0)
            {
                ip_[0] = '\0';
            }

            SimpleTCP::SimpleTCP(const char *ip, uint16_t port)
                : m_sock(-1), port_(port)
            {
                if (ip != nullptr)
                {
                    // Copia segura estándar
                    strncpy(ip_, ip, sizeof(ip_) - 1);
                    ip_[sizeof(ip_) - 1] = '\0';
                }
                else
                {
                    ip_[0] = '\0';
                    FP_LOG_W(TAG, "Constructor de cliente con IP nula.");
                }
            }

            SimpleTCP::~SimpleTCP()
            {
                close();
                FP_LOG_I(TAG, "Canal destruido.");
            }

            void SimpleTCP::open()
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);

                if (isRunning_.load())
                {
                    FP_LOG_W(TAG, "Hilo de eventos ya iniciado.");
                    return;
                }

                // --- Lógica de conexión CLIENTE ---
                if (m_sock == -1)
                {
                    if (ip_[0] == '\0' || port_ == 0)
                    {
                        FP_LOG_E(TAG, "No se puede abrir: IP/Puerto no configurados.");
                        return;
                    }

                    FP_LOG_I(TAG, "Conectando a %s:%u...", ip_, port_);

                    struct addrinfo hints = {}, *res = nullptr;
                    hints.ai_family = AF_INET;
                    hints.ai_socktype = SOCK_STREAM;
                    hints.ai_protocol = IPPROTO_TCP;

                    char port_str[6];
                    snprintf(port_str, sizeof(port_str), "%u", port_);

                    int err = getaddrinfo(ip_, port_str, &hints, &res);
                    if (err != 0)
                    {
                        FP_LOG_E(TAG, "getaddrinfo failed: %s", gai_strerror(err));
                        return;
                    }

                    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                    if (sock < 0)
                    {
                        FP_LOG_E(TAG, "Error creando socket: %s", strerror(errno));
                        freeaddrinfo(res);
                        return;
                    }

                    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
                    {
                        FP_LOG_E(TAG, "Error en connect: %s", strerror(errno));
                        ::close(sock);
                        freeaddrinfo(res);
                        return;
                    }

                    freeaddrinfo(res);
                    FP_LOG_I(TAG, "Conectado! Socket fd: %d", sock);
                    m_sock = sock;
                }

                // --- Iniciar hilo de lectura ---
                isRunning_.store(true);
                try
                {
                    std::thread([self = shared_from_this()]()
                                { self->eventThreadFunc(); })
                        .detach();
                }
                catch (...)
                {
                    isRunning_.store(false);
                    if (m_sock != -1)
                    {
                        ::close(m_sock);
                        m_sock = -1;
                    }
                    FP_LOG_E(TAG, "Error al crear hilo de eventos.");
                }
            }

            void SimpleTCP::close()
            {
                int sock_to_shutdown = -1;
                {
                    std::lock_guard<std::recursive_mutex> lock(mutex_);
                    if (m_sock != -1)
                    {
                        sock_to_shutdown = m_sock;
                    }
                }

                if (sock_to_shutdown != -1)
                {
                    FP_LOG_I(TAG, "Solicitando cierre (shutdown)...");
                    // SHUT_RDWR cierra lectura y escritura
                    shutdown(sock_to_shutdown, SHUT_RDWR);
                    // Nota: En Linux, shutdown despierta al recv, pero el cierre real
                    // del descriptor lo hacemos al final del hilo o en el destructor.
                }
            }

            void SimpleTCP::send(const uint8_t *data, size_t len)
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                if (m_sock == -1)
                    return;
                if (!data || len == 0)
                    return;

                size_t total_sent = 0;
                while (total_sent < len)
                {
                    // send en Linux usa flag MSG_NOSIGNAL para evitar señal SIGPIPE si el otro lado cierra
                    ssize_t sent_now = ::send(m_sock, (const char *)(data + total_sent), (len - total_sent), MSG_NOSIGNAL);
                    
                    if (sent_now < 0)
                    {
                        FP_LOG_E(TAG, "Error en send(): %s", strerror(errno));
                        shutdown(m_sock, SHUT_RDWR);
                        return;
                    }
                    total_sent += sent_now;
                }
            }

            void SimpleTCP::eventThreadFunc()
            {
                if (onOpen)
                    onOpen();

                FP_LOG_I(TAG, "Hilo iniciado.");
                std::vector<char> rx_buffer(4096);

                while (isRunning_.load())
                {
                    ssize_t len = recv(m_sock, rx_buffer.data(), rx_buffer.size(), 0);
                    if (len > 0)
                    {
                        if (onData)
                            onData((uint8_t *)rx_buffer.data(), (size_t)len);
                    }
                    else if (len == 0)
                    {
                        FP_LOG_I(TAG, "Conexión cerrada por el peer.");
                        break;
                    }
                    else
                    {
                        // Error, a menos que sea interrumpido
                        if (errno != EINTR) {
                             FP_LOG_D(TAG, "recv salió con error: %s", strerror(errno));
                        }
                        break;
                    }
                }

                // Limpieza
                {
                    std::lock_guard<std::recursive_mutex> lock(mutex_);
                    if (m_sock != -1)
                    {
                        ::close(m_sock);
                        m_sock = -1;
                    }
                    isRunning_.store(false);
                }

                if (onClose)
                    onClose();
                FP_LOG_I(TAG, "Hilo terminado.");
            }
        }
    }
}
