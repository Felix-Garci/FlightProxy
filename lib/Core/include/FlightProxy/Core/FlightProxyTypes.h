#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace FlightProxy
{
    namespace Types
    {
        struct MspPacket
        {
            char direction;
            uint8_t command;
            std::vector<uint8_t> payload;
        };

        struct IBUSPacket
        {
            static constexpr size_t NUM_CHANNELS = 14;
            std::array<uint16_t, NUM_CHANNELS> channels;
        };
    }
}