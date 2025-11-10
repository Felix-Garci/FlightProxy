#pragma once

#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/FlightProxyTypes.h" // Asumo que aquí está definido IBUSPacket

#include <cstdint>
#include <vector>
#include <array>
#include <memory>

namespace FlightProxy
{
    namespace Core
    {
        namespace Protocol
        {
            // --- Constantes y detalles de implementación de IBUS ---
            namespace Detail
            {
                static constexpr uint8_t IBUS_HEADER = 0x20;
                static constexpr uint8_t IBUS_CMD_SERVO = 0x40;
                static constexpr size_t IBUS_PACKET_SIZE = 32;
                static constexpr size_t IBUS_PAYLOAD_SIZE = 28; // 14 canales * 2 bytes
            }

            /**
             * @brief Encoder para IBUS (FlySky Standard Servo Protocol)
             * Genera paquetes de 32 bytes: [0x20][0x40][CH1_L][CH1_H]...[CH14_H][CHK_L][CHK_H]
             */
            class IbusEncoder : public IEncoderT<FlightProxy::Core::IBUSPacket>
            {
            public:
                std::vector<uint8_t> encode(std::unique_ptr<const FlightProxy::Core::IBUSPacket> packet) override
                {
                    std::vector<uint8_t> buffer(Detail::IBUS_PACKET_SIZE);

                    // 1. Cabeceras
                    buffer[0] = Detail::IBUS_HEADER;
                    buffer[1] = Detail::IBUS_CMD_SERVO;

                    // 2. Payload (Canales en Little-Endian)
                    uint16_t checksum_sum = buffer[0] + buffer[1];
                    size_t offset = 2;
                    for (size_t i = 0; i < FlightProxy::Core::IBUSPacket::NUM_CHANNELS; ++i)
                    {
                        uint16_t val = packet->channels[i];
                        uint8_t low = static_cast<uint8_t>(val & 0xFF);
                        uint8_t high = static_cast<uint8_t>((val >> 8) & 0xFF);

                        buffer[offset++] = low;
                        buffer[offset++] = high;

                        checksum_sum += low + high;
                    }

                    // 3. Calcular Checksum IBUS (0xFFFF - suma de los primeros 30 bytes)
                    uint16_t checksum = 0xFFFF - checksum_sum;

                    // 4. Insertar Checksum (Little-Endian) al final (bytes 30 y 31)
                    buffer[30] = static_cast<uint8_t>(checksum & 0xFF);
                    buffer[31] = static_cast<uint8_t>((checksum >> 8) & 0xFF);

                    return buffer;
                }
            };

            /**
             * @brief Decoder para IBUS.
             * Máquina de estados robusta para detectar tramas válidas de 32 bytes empezando por 0x20 0x40.
             */
            class IbusDecoder : public IDecoderT<FlightProxy::Core::IBUSPacket>
            {
            private:
                enum class ParseState
                {
                    IDLE,       // Esperando 0x20
                    CMD,        // Esperando 0x40
                    PAYLOAD,    // Leyendo los 28 bytes de canales
                    CHECKSUM_L, // Leyendo byte bajo del checksum
                    CHECKSUM_H  // Leyendo byte alto del checksum y validando
                };

                ParseState state_ = ParseState::IDLE;
                uint16_t current_checksum_sum_ = 0; // Acumulador para calcular el checksum sobre la marcha
                uint8_t payload_index_ = 0;
                uint8_t raw_payload_[Detail::IBUS_PAYLOAD_SIZE]; // Buffer temporal para los datos crudos
                uint16_t received_checksum_ = 0;

                std::function<void(std::unique_ptr<const FlightProxy::Core::IBUSPacket>)> onPacketHandler_;

                void parse(uint8_t byte)
                {
                    switch (state_)
                    {
                    case ParseState::IDLE:
                        if (byte == Detail::IBUS_HEADER)
                        {
                            state_ = ParseState::CMD;
                            current_checksum_sum_ = byte; // Iniciar suma con la cabecera
                        }
                        break;

                    case ParseState::CMD:
                        if (byte == Detail::IBUS_CMD_SERVO)
                        {
                            state_ = ParseState::PAYLOAD;
                            current_checksum_sum_ += byte;
                            payload_index_ = 0;
                        }
                        else
                        {
                            // Si no es 0x40, quizás era un nuevo 0x20 o basura. Reiniciamos.
                            state_ = (byte == Detail::IBUS_HEADER) ? ParseState::CMD : ParseState::IDLE;
                            current_checksum_sum_ = (byte == Detail::IBUS_HEADER) ? byte : 0;
                        }
                        break;

                    case ParseState::PAYLOAD:
                        raw_payload_[payload_index_++] = byte;
                        current_checksum_sum_ += byte;

                        if (payload_index_ == Detail::IBUS_PAYLOAD_SIZE)
                        {
                            state_ = ParseState::CHECKSUM_L;
                        }
                        break;

                    case ParseState::CHECKSUM_L:
                        received_checksum_ = byte;
                        state_ = ParseState::CHECKSUM_H;
                        break;

                    case ParseState::CHECKSUM_H:
                        received_checksum_ |= (static_cast<uint16_t>(byte) << 8);

                        // Validar Checksum
                        if (received_checksum_ == (uint16_t)(0xFFFF - current_checksum_sum_))
                        {
                            if (onPacketHandler_)
                            {
                                auto packet = std::make_unique<FlightProxy::Core::IBUSPacket>();
                                // Deserializar el payload a la estructura de canales
                                for (size_t i = 0; i < FlightProxy::Core::IBUSPacket::NUM_CHANNELS; ++i)
                                {
                                    packet->channels[i] = raw_payload_[i * 2] | (static_cast<uint16_t>(raw_payload_[i * 2 + 1]) << 8);
                                }
                                onPacketHandler_(std::move(packet));
                            }
                        }

                        // Siempre volvemos a IDLE tras completar un paquete (válido o no)
                        reset();
                        break;
                    }
                }

            public:
                IbusDecoder()
                {
                    reset();
                }

                void feed(const uint8_t *data, size_t len) override
                {
                    for (size_t i = 0; i < len; ++i)
                    {
                        parse(data[i]);
                    }
                }

                void onPacket(std::function<void(std::unique_ptr<const FlightProxy::Core::IBUSPacket>)> handler) override
                {
                    onPacketHandler_ = handler;
                }

                void reset() override
                {
                    state_ = ParseState::IDLE;
                    current_checksum_sum_ = 0;
                    payload_index_ = 0;
                    received_checksum_ = 0;
                }
            };
        }
    }
}