#include "FlightProxy/PlatformWin/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <cstring>
#include <iostream>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            static const char *TAG = "SimpleTCP_Win";

            // Inicialización estática de contadores Winsock
            int SimpleTCP::winsockInitCount_ = 0;
            std::mutex SimpleTCP::winsockMutex_;

            void SimpleTCP::initWinsock()
            {
                std::lock_guard lock(winsockMutex_);
                if (winsockInitCount_ == 0)
                {
                    WSADATA wsaData;
                    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
                    if (iResult != 0)
                    {
                        FP_LOG_E(TAG, "WSAStartup failed: %d", iResult);
                    }
                }
                winsockInitCount_++;
            }

            void SimpleTCP::cleanupWinsock()
            {
                std::lock_guard lock(winsockMutex_);
                winsockInitCount_--;
                if (winsockInitCount_ == 0)
                {
                    WSACleanup();
                }
            }

            SimpleTCP::SimpleTCP(SOCKET accepted_socket)
                : m_sock(accepted_socket), port_(0)
            {
                initWinsock();
                ip_[0] = '\0';
            }

            SimpleTCP::SimpleTCP(const char *ip, uint16_t port)
                : m_sock(INVALID_SOCKET), port_(port)
            {
                initWinsock();
                if (ip != nullptr)
                {
                    strncpy_s(ip_, sizeof(ip_), ip, _TRUNCATE);
                }
                else
                {
                    ip_[0] = '\0';
                    FP_LOG_W(TAG, "Constructor de cliente con IP nula.");
                }
            }

            SimpleTCP::~SimpleTCP()
            {
                close(); // Asegurar cierre
                cleanupWinsock();
                FP_LOG_I(TAG, "Canal destruido.");
            }

            void SimpleTCP::open()
            {
                std::lock_guard lock(mutex_);

                if (isRunning_.load())
                {
                    FP_LOG_W(TAG, "Hilo de eventos ya iniciado.");
                    return;
                }

                // --- Lógica de conexión CLIENTE ---
                if (m_sock == INVALID_SOCKET)
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
                    sprintf_s(port_str, "%u", port_);

                    int err = getaddrinfo(ip_, port_str, &hints, &res);
                    if (err != 0)
                    {
                        FP_LOG_E(TAG, "getaddrinfo failed: %d", err);
                        return;
                    }

                    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                    if (sock == INVALID_SOCKET)
                    {
                        FP_LOG_E(TAG, "Error creando socket: %d", WSAGetLastError());
                        freeaddrinfo(res);
                        return;
                    }

                    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR)
                    {
                        FP_LOG_E(TAG, "Error en connect: %d", WSAGetLastError());
                        closesocket(sock);
                        freeaddrinfo(res);
                        return;
                    }

                    freeaddrinfo(res);
                    FP_LOG_I(TAG, "Conectado! Socket: %lld", sock);
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
                    if (m_sock != INVALID_SOCKET)
                    {
                        closesocket(m_sock);
                        m_sock = INVALID_SOCKET;
                    }
                    FP_LOG_E(TAG, "Error al crear hilo de eventos.");
                }
            }

            void SimpleTCP::close()
            {
                // No bloqueamos el mutex aquí para permitir que shutdown despierte al recv
                SOCKET sock_to_shutdown = INVALID_SOCKET;
                {
                    std::lock_guard lock(mutex_);
                    if (m_sock != INVALID_SOCKET)
                    {
                        sock_to_shutdown = m_sock;
                    }
                }

                if (sock_to_shutdown != INVALID_SOCKET)
                {
                    FP_LOG_I(TAG, "Solicitando cierre (shutdown)...");
                    shutdown(sock_to_shutdown, SD_BOTH);
                }
            }

            void SimpleTCP::send(const uint8_t *data, size_t len)
            {
                std::lock_guard lock(mutex_);
                if (m_sock == INVALID_SOCKET)
                    return;
                if (!data || len == 0)
                    return;

                size_t total_sent = 0;
                while (total_sent < len)
                {
                    int sent_now = ::send(m_sock, (const char *)(data + total_sent), (int)(len - total_sent), 0);
                    if (sent_now == SOCKET_ERROR)
                    {
                        FP_LOG_E(TAG, "Error en send(): %d", WSAGetLastError());
                        shutdown(m_sock, SD_BOTH);
                        return;
                    }
                    total_sent += sent_now;
                }
            }

            void SimpleTCP::eventThreadFunc()
            {
                // Mantenemos una referencia shared_ptr a nosotros mismos para evitar destrucción prematura
                // (Esto ya se pasa implícitamente si usas std::bind o lambdas que capturan shared_ptr,
                //  pero aquí lo hicimos pasando shared_from_this() al constructor del thread)

                if (onOpen)
                    onOpen();

                FP_LOG_I(TAG, "Hilo iniciado.");
                std::vector<char> rx_buffer(4096);

                while (isRunning_.load())
                {
                    int len = recv(m_sock, rx_buffer.data(), (int)rx_buffer.size(), 0);
                    if (len > 0)
                    {
                        if (onData)
                            onData((uint8_t *)rx_buffer.data(), len);
                    }
                    else if (len == 0)
                    {
                        FP_LOG_I(TAG, "Conexión cerrada por el peer.");
                        break;
                    }
                    else
                    {
                        break;
                    }
                }

                // Limpieza
                {
                    std::lock_guard lock(mutex_);
                    if (m_sock != INVALID_SOCKET)
                    {
                        closesocket(m_sock);
                        m_sock = INVALID_SOCKET;
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