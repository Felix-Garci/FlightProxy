#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Core
    {
        namespace Protocol
        {
            template <typename PacketT>
            class IDecoderT
            {
            public:
                virtual ~IDecoderT() = default;
                virtual void feed(const uint8_t *data, size_t len) = 0;
                virtual void onPacket(std::function<void(std::unique_ptr<const PacketT>)> handler) = 0;
                virtual void reset() = 0;
            };
        }
    }
}