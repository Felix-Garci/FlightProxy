#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>

// Necesario para linkar con la librería de Winsock
#pragma comment(lib, "Ws2_32.lib")

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            class SimpleTCP : public FlightProxy::Core::Transport::ITransport,
                              public std::enable_shared_from_this<SimpleTCP>
            {
            public:
                SimpleTCP(SOCKET accepted_socket);
                SimpleTCP(const char *ip, uint16_t port);
                ~SimpleTCP() override;

                void open() override;
                void close() override;
                void send(const uint8_t *data, size_t len) override;

            private:
                SOCKET m_sock = INVALID_SOCKET;
                uint16_t port_ = 0;
                char ip_[16] = {0};

                std::recursive_mutex mutex_;
                std::atomic<bool> isRunning_{false}; // Para controlar el bucle del hilo de forma limpia

                void eventThreadFunc(); // Función miembro para el hilo

                // Ayuda para inicializar Winsock solo una vez si es necesario
                static void initWinsock();
                static void cleanupWinsock();
                static int winsockInitCount_;
                static std::mutex winsockMutex_;
            };
        }
    }
}