#pragma once
#include "FlightProxy/Core/Utils/Logger.h"

#include "esp_debug_helpers.h"

#include <cstdint>
#include <vector>
#include <array>

namespace FlightProxy
{
    namespace Core
    {
        struct MspPacket
        {
            char direction;
            uint16_t command;
            std::vector<uint8_t> payload;
            MspPacket(char dir, uint8_t cmd, std::vector<uint8_t> pld) : direction(dir), command(cmd), payload(std::move(pld))
            {
                FP_LOG_I("MspPacket", "Creado con valores en %p", this);
                // esp_backtrace_print(10);
            }
            MspPacket()
            {
                FP_LOG_I("MspPacket", "Creado vacio");
                // esp_backtrace_print(10);
            }
            ~MspPacket()
            {
                FP_LOG_I("MspPacket", "Destruido MspPacket en %p", this);
                // esp_backtrace_print(10);
            }
            MspPacket(const MspPacket &other) : command(other.command), payload(other.payload)
            {
                FP_LOG_I("MspPacket", "Copiado MspPacket a %p desde %p", this, &other);
                // esp_backtrace_print(10);
            }
        };

        struct IBUSPacket
        {
            static constexpr size_t NUM_CHANNELS = 14;
            std::array<uint16_t, NUM_CHANNELS> channels;
        };

        template <typename PacketT>
        struct PacketEnvelope
        {
            const PacketT *raw_packet_ptr; // El paquete de datos real
            uint32_t channelId;            // ID del canal para saber a quién responder
        };

    }
}