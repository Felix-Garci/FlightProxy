#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Mocks/HostLogger.h"
#include <vector>
#include <cstdint>

namespace FlightProxy
{
    namespace Mocks
    {
        const char *TAG = "Mock Transport";
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
                FP_LOG_D(TAG, "data recived %d", len);
                simulateRx(data, len);
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