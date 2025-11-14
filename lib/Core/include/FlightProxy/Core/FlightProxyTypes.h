#pragma once
#include "FlightProxy/Core/Utils/Logger.h"

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
            using ChannelsT = std::array<uint16_t, NUM_CHANNELS>;
            ChannelsT channels;
        };

        template <typename PacketT>
        struct PacketEnvelope
        {
            const PacketT *raw_packet_ptr; // El paquete de datos real
            uint32_t channelId;            // ID del canal para saber a quién responder
        };

        // Tipos de datos comunes para el sistema
        struct RCData
        {
            // 4 canales principales
            uint16_t roll;
            uint16_t pitch;
            uint16_t throttle;
            uint16_t yaw;

            // Canales auxiliares primarios
            uint16_t aux1;
            uint16_t aux2;

            // Canales auxiliares 8 secundarios
            std::array<uint16_t, 8> aux_channels;
        };

        struct GPSData
        {
            double latitude;
            double longitude;
            float altitude;
            float speed;
            float heading;
        };

        struct MagData
        {
            float mag_x;
            float mag_y;
            float mag_z;
        };

        struct BaroData
        {
            float pressure;
            float altitude;
            float temperature;
        };

        struct IMUData
        {
            // COn singo
            int16_t accel_x;
            int16_t accel_y;
            int16_t accel_z;
            int16_t gyro_x;
            int16_t gyro_y;
            int16_t gyro_z;
        };

    }
}