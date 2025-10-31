#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include <vector>
#include <cstdint>

namespace FlightProxy
{
    namespace Mocks
    {

        class MockTransport : public Core::Transport::ITransport
        {
        public:
            bool isOpen = false;
            std::vector<uint8_t> lastSentData;
            int sendCount = 0;

            void open() override
            {
                isOpen = true;
                if (onOpen)
                    onOpen();
            }
            void close() override
            {
                isOpen = false;
                if (onClose)
                    onClose();
            }
            void send(const uint8_t *data, size_t len) override
            {
                sendCount++;
                lastSentData.assign(data, data + len);
            }

            // Función especial para simular datos ENTRANDES
            void simulateRx(const uint8_t *data, size_t len)
            {
                if (onData)
                {
                    onData(data, len);
                }
            }
        };

    }
}