#pragma once
#include <cstdint>
#include <functional>
/*
interface ITransport{
    {field}+onOpen: std::function<void()>
    {field}+onData: std::function<void(const uint8_t*, size_t)>
    {field}+onClose: std::function<void()>

    +~ITransport()
    +open(): void
    +close(): void
    +send(data:const uint8_t*,len:size_t): void
}
*/
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