#include "FlightProxy/PlatformWin/Transport/ListenerTCP.h"
#include "FlightProxy/PlatformWin/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <iostream>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            static const char *TAG = "ListenerTCP_Win";

            // Inicialización estática de contadores Winsock
            int ListenerTCP::winsockInitCount_ = 0;
            std::mutex ListenerTCP::winsockMutex_;

            void ListenerTCP::initWinsock()
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

            void ListenerTCP::cleanupWinsock()
            {
                std::lock_guard lock(winsockMutex_);
                winsockInitCount_--;
                if (winsockInitCount_ == 0)
                {
                    WSACleanup();
                }
            }

            ListenerTCP::ListenerTCP()
            {
                initWinsock();
            }

            ListenerTCP::~ListenerTCP()
            {
                stopListening();

                // Esperar a que el hilo termine
                if (m_listener_thread.joinable())
                {
                    m_listener_thread.join();
                }

                cleanupWinsock();
                FP_LOG_I(TAG, "Listener destruido.");
            }

            bool ListenerTCP::startListening(uint16_t port)
            {
                std::lock_guard lock(m_mutex);

                if (m_is_running.load())
                {
                    FP_LOG_W(TAG, "El listener ya estaba iniciado.");
                    return true;
                }

                // 1. Crear socket
                m_server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (m_server_sock == INVALID_SOCKET)
                {
                    FP_LOG_E(TAG, "Error creando socket: %d", WSAGetLastError());
                    return false;
                }

                // 2. Bind
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = INADDR_ANY;
                server_addr.sin_port = htons(port);

                if (bind(m_server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
                {
                    FP_LOG_E(TAG, "Error en bind: %d", WSAGetLastError());
                    closesocket(m_server_sock);
                    m_server_sock = INVALID_SOCKET;
                    return false;
                }

                // 3. Listen
                if (listen(m_server_sock, SOMAXCONN) == SOCKET_ERROR)
                {
                    FP_LOG_E(TAG, "Error en listen: %d", WSAGetLastError());
                    closesocket(m_server_sock);
                    m_server_sock = INVALID_SOCKET;
                    return false;
                }

                // 4. Iniciar hilo
                m_is_running.store(true);
                try
                {
                    m_listener_thread = std::thread(&ListenerTCP::listenerThreadFunc, this);
                }
                catch (...)
                {
                    m_is_running.store(false);
                    closesocket(m_server_sock);
                    m_server_sock = INVALID_SOCKET;
                    FP_LOG_E(TAG, "Error al crear el hilo del listener");
                    return false;
                }

                FP_LOG_I(TAG, "Listener iniciado en puerto %d", port);
                return true;
            }

            void ListenerTCP::stopListening()
            {
                // Copia local del socket para cerrarlo fuera del lock si queremos
                SOCKET sock_to_close = INVALID_SOCKET;
                {
                    std::lock_guard lock(m_mutex);
                    if (!m_is_running.load())
                        return;

                    m_is_running.store(false);
                    sock_to_close = m_server_sock;
                    // No invalidamos m_server_sock aquí para que el hilo sepa qué cerrar si llegamos antes,
                    // aunque en este diseño el hilo lo cerrará al salir o nosotros aquí para despertarlo.
                }

                if (sock_to_close != INVALID_SOCKET)
                {
                    // Cerrar el socket despertará al accept() con un error
                    closesocket(sock_to_close);
                }
            }

            void ListenerTCP::listenerThreadFunc()
            {
                FP_LOG_I(TAG, "Hilo de Listener iniciado.");

                while (m_is_running.load())
                {
                    struct sockaddr_in client_addr;
                    int addr_len = sizeof(client_addr);

                    // accept() es bloqueante
                    SOCKET client_sock = accept(m_server_sock, (struct sockaddr *)&client_addr, &addr_len);

                    if (client_sock == INVALID_SOCKET)
                    {
                        if (m_is_running.load())
                        {
                            // Error real solo si seguimos queriendo correr
                            FP_LOG_E(TAG, "Error en accept: %d", WSAGetLastError());
                        }
                        else
                        {
                            // Fue provocado por stopListening()
                            FP_LOG_I(TAG, "Listener detenido voluntariamente.");
                        }
                        break;
                    }

                    FP_LOG_I(TAG, "Cliente conectado! Socket: %lld", client_sock);

                    auto new_transport = std::make_shared<SimpleTCP>(client_sock);

                    if (onNewTransport)
                    {
                        onNewTransport(new_transport);
                    }
                }

                // Limpieza final del hilo
                {
                    std::lock_guard lock(m_mutex);
                    if (m_server_sock != INVALID_SOCKET)
                    {
                        closesocket(m_server_sock);
                        m_server_sock = INVALID_SOCKET;
                    }
                    m_is_running.store(false);
                }
                FP_LOG_I(TAG, "Hilo de Listener terminado.");
            }

        } // namespace Transport
    }
} // namespace FlightProxy