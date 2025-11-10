#include "FlightProxy/PlatformWin/Transport/SimpleUDP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <iostream>
#include <vector>
#include <cstring>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            static const char *TAG = "SimpleUDP_Win";

            int SimpleUDP::winsockInitCount_ = 0;
            std::mutex SimpleUDP::winsockMutex_;

            void SimpleUDP::initWinsock()
            {
                std::lock_guard lock(winsockMutex_);
                if (winsockInitCount_ == 0)
                {
                    WSADATA wsaData;
                    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
                    if (iResult != 0)
                        FP_LOG_E(TAG, "WSAStartup failed: %d", iResult);
                }
                winsockInitCount_++;
            }

            void SimpleUDP::cleanupWinsock()
            {
                std::lock_guard lock(winsockMutex_);
                winsockInitCount_--;
                if (winsockInitCount_ == 0)
                    WSACleanup();
            }

            SimpleUDP::SimpleUDP(uint16_t port)
                : m_port(port), m_last_sender_len(sizeof(m_last_sender_addr))
            {
                initWinsock();
                memset(&m_last_sender_addr, 0, sizeof(m_last_sender_addr));
            }

            SimpleUDP::~SimpleUDP()
            {
                close();
                cleanupWinsock();
                FP_LOG_I(TAG, "Canal UDP destruido.");
            }

            void SimpleUDP::open()
            {
                std::lock_guard lock(mutex_);

                if (isRunning_.load())
                {
                    FP_LOG_W(TAG, "Hilo de eventos ya iniciado.");
                    return;
                }

                if (m_sock == INVALID_SOCKET)
                {
                    FP_LOG_I(TAG, "Intentando escuchar en UDP:%u...", m_port);

                    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if (m_sock == INVALID_SOCKET)
                    {
                        FP_LOG_E(TAG, "Error creando socket UDP: %d", WSAGetLastError());
                        return;
                    }

                    struct sockaddr_in bind_addr;
                    memset(&bind_addr, 0, sizeof(bind_addr));
                    bind_addr.sin_family = AF_INET;
                    bind_addr.sin_port = htons(m_port);
                    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

                    if (bind(m_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR)
                    {
                        FP_LOG_E(TAG, "Error en bind UDP: %d", WSAGetLastError());
                        closesocket(m_sock);
                        m_sock = INVALID_SOCKET;
                        return;
                    }
                    FP_LOG_I(TAG, "Escuchando en UDP:%u! Socket: %lld", m_port, m_sock);
                }

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
                    closesocket(m_sock);
                    m_sock = INVALID_SOCKET;
                    FP_LOG_E(TAG, "Error al crear hilo UDP.");
                }
            }

            void SimpleUDP::close()
            {
                // Para UDP en Windows, shutdown puede no ser suficiente para despertar recvfrom en todas las versiones/configuraciones.
                // Cerrar el socket directamente suele ser necesario para forzar la salida del hilo bloqueado.
                SOCKET sock_to_close = INVALID_SOCKET;
                {
                    std::lock_guard lock(mutex_);
                    if (m_sock != INVALID_SOCKET)
                    {
                        sock_to_close = m_sock;
                        // No invalidamos m_sock aquí para que el hilo lo sepa cerrar limpiamente si llegamos antes.
                    }
                    isRunning_.store(false);
                }

                if (sock_to_close != INVALID_SOCKET)
                {
                    closesocket(sock_to_close);
                }
            }

            void SimpleUDP::send(const uint8_t *data, size_t len)
            {
                std::lock_guard lock(mutex_);
                if (m_sock == INVALID_SOCKET || !m_has_last_sender || !data || len == 0)
                    return;

                int sent = sendto(m_sock, (const char *)data, (int)len, 0,
                                  (struct sockaddr *)&m_last_sender_addr, m_last_sender_len);

                if (sent == SOCKET_ERROR)
                {
                    FP_LOG_E(TAG, "Error sendto UDP: %d", WSAGetLastError());
                }
            }

            void SimpleUDP::eventThreadFunc()
            {
                if (onOpen)
                    onOpen();

                FP_LOG_I(TAG, "Hilo UDP iniciado.");
                std::vector<char> rx_buffer(1500); // MTU típica

                while (isRunning_.load())
                {
                    struct sockaddr_in sender_addr;
                    int sender_len = sizeof(sender_addr);

                    int len = recvfrom(m_sock, rx_buffer.data(), (int)rx_buffer.size(), 0,
                                       (struct sockaddr *)&sender_addr, &sender_len);

                    if (len > 0)
                    {
                        {
                            std::lock_guard lock(mutex_);
                            m_last_sender_addr = sender_addr;
                            m_last_sender_len = sender_len;
                            m_has_last_sender = true;
                        }

                        if (onData)
                        {
                            // Opcional: log de IP remitente
                            // char ip_str[INET_ADDRSTRLEN];
                            // inet_ntop(AF_INET, &sender_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
                            // FP_LOG_I(TAG, "UDP de %s:%u", ip_str, ntohs(sender_addr.sin_port));
                            onData((uint8_t *)rx_buffer.data(), len);
                        }
                    }
                    else
                    {
                        // Error o cierre. Si fue provocado por close(), WSAGetLastError puede ser WSAEINTR o similar.
                        if (isRunning_.load())
                        {
                            FP_LOG_E(TAG, "Error recvfrom: %d", WSAGetLastError());
                        }
                        break;
                    }
                }

                {
                    std::lock_guard lock(mutex_);
                    if (m_sock != INVALID_SOCKET)
                    {
                        closesocket(m_sock);
                        m_sock = INVALID_SOCKET;
                    }
                    m_has_last_sender = false;
                    isRunning_.store(false);
                }

                if (onClose)
                    onClose();
                FP_LOG_I(TAG, "Hilo UDP terminado.");
            }
        }
    }
}