#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "rc/RcTypes.hpp"

namespace fcbridge::ibus
{
    class IbusFrame
    {
    public:
        static constexpr uint8_t FRAME_LEN = 32;
        static constexpr uint8_t NUM_CHANNELS = 14;
        static constexpr uint8_t HEADER_LEN = 2;

        static bool encode(std::span<const uint16_t, NUM_CHANNELS> channels,
                           std::span<uint8_t, FRAME_LEN> outBuffer)
        {
            if (outBuffer.size() < FRAME_LEN)
                return false;

            outBuffer[0] = FRAME_LEN;
            outBuffer[1] = 0x40;

            // Canales (14 x 2 bytes)
            for (size_t i = 0; i < NUM_CHANNELS; ++i)
            {
                const uint16_t v = channels[i];
                outBuffer[2 + i * 2] = static_cast<uint8_t>(v & 0xFF);
                outBuffer[2 + i * 2 + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            }

            // Calcular checksum
            uint16_t sum = 0;
            for (size_t i = 0; i < FRAME_LEN - 2; ++i)
                sum += outBuffer[i];
            uint16_t checksum = 0xFFFF - sum;

            outBuffer[FRAME_LEN - 2] = static_cast<uint8_t>(checksum & 0xFF);
            outBuffer[FRAME_LEN - 1] = static_cast<uint8_t>((checksum >> 8) & 0xFF);

            return true;
        }
    };

} // namespace fcbridge::ibus
