#pragma once
#include <cstdint>
#include <functional>

namespace FlightProxy
{
    namespace Transport
    {
        class ITransport
        {
        public:
            virtual ~ITransport() = default;

            virtual void open() = 0;
            virtual void close() = 0;
            virtual void send(const uint8_t *data, size_t len) = 0;

            std::function<void()> onOpen;
            std::function<void(const uint8_t *, size_t)> onData;
            std::function<void()> onClose;
        };

    }
}