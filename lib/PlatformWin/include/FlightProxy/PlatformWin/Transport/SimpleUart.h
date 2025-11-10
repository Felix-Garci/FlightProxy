#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            /**
             * @brief Versión MOCK de SimpleUART para PC.
             * No usa hardware real. Actúa como un loopback: todo lo que envías
             * se recibe inmediatamente de vuelta a través de onData.
             * Útil para pruebas y desarrollo sin hardware.
             */
            class SimpleUart : public FlightProxy::Core::Transport::ITransport
            {
            public:
                // Mantenemos la firma del constructor real por compatibilidad,
                // pero ignoraremos los parámetros.
                SimpleUart(const std::string &portName, uint32_t baudRate);
                virtual ~SimpleUart() = default;

                void open() override;
                void close() override;
                void send(const uint8_t *data, size_t len) override;

                // --- Miembros para inspección en Tests ---
                bool isOpen = false;
                std::vector<uint8_t> lastSentData;
                int sendCount = 0;

            private:
                std::recursive_mutex mutex_; // Para hacer el mock thread-safe
            };
        }
    }
}