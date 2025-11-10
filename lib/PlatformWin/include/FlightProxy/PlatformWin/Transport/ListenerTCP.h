#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

// Necesario para linkar con la librería de Winsock
#pragma comment(lib, "Ws2_32.lib")

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            class ListenerTCP : public Core::Transport::ITcpListener,
                                public std::enable_shared_from_this<ListenerTCP>
            {
            public:
                ListenerTCP();
                virtual ~ListenerTCP() override;

                bool startListening(uint16_t port) override;
                void stopListening() override;

            private:
                void listenerThreadFunc(); // Función del hilo

                std::thread m_listener_thread;
                SOCKET m_server_sock = INVALID_SOCKET;
                std::recursive_mutex m_mutex;
                std::atomic<bool> m_is_running{false};

                // Gestión de Winsock (igual que en SimpleTCP)
                static void initWinsock();
                static void cleanupWinsock();
                static int winsockInitCount_;
                static std::mutex winsockMutex_;
            };

        } // namespace Transport
    }
} // namespace FlightProxy