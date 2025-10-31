#pragma once
#include <cstdint>
#include <vector>

namespace FlightProxy
{
    namespace Core
    {
        namespace Protocol
        {
            template <typename PacketT>
            class IEncoderT
            {
            public:
                virtual ~IEncoderT() = default;
                virtual std::vector<uint8_t> encode(const PacketT &packet) = 0;
            };
        }
    }
}