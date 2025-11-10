#pragma once
#include <cstdint>
#include <vector>
#include <memory>

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
                virtual std::vector<uint8_t> encode(std::unique_ptr<const PacketT> packet) = 0;
            };
        }
    }
}