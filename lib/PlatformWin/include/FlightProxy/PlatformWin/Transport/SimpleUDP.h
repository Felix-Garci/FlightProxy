#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
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
            class SimpleUDP : public FlightProxy::Core::Transport::ITransport,
                              public std::enable_shared_from_this<SimpleUDP>
            {
            public:
                SimpleUDP(uint16_t port);
                ~SimpleUDP() override;

                void open() override;
                void close() override;
                void send(const uint8_t *data, size_t len) override;

            private:
                SOCKET m_sock = INVALID_SOCKET;
                uint16_t m_port;

                // Para responder al último remitente
                struct sockaddr_in m_last_sender_addr;
                int m_last_sender_len;
                bool m_has_last_sender = false;

                std::recursive_mutex mutex_;
                std::atomic<bool> isRunning_{false};

                void eventThreadFunc();

                // Gestión de Winsock
                static void initWinsock();
                static void cleanupWinsock();
                static int winsockInitCount_;
                static std::mutex winsockMutex_;
            };
        }
    }
}