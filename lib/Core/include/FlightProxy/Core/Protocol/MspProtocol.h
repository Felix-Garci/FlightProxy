#pragma once
#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

#include <cstdint>
#include <vector>

namespace FlightProxy
{
    namespace Core
    {
        namespace Protocol
        {
            // Comandos MSP
            const uint16_t MSP_IMU_DATA = 105;

            // --- Detalle de implementación de MSP V2 (CRC8) ---
            namespace Detail
            {
                // Tabla CRC8 para DVB-S2 (Polinomio 0xD5), usado en MSP V2

                static const uint8_t crc8_dvb_s2_table[256] = {
                    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
                    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
                    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
                    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
                    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
                    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
                    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
                    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
                    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
                    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
                    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
                    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
                    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
                    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
                    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
                    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

                // Calcula un byte de CRC
                inline uint8_t crc8_dvb_s2(uint8_t crc, uint8_t a)
                {
                    return crc8_dvb_s2_table[crc ^ a];
                }
            } // namespace Detail

            /**
             * @brief Encoder para MSP V2
             */
            class MspEncoder : public IEncoderT<FlightProxy::Core::MspPacket>
            {
            public:
                std::vector<uint8_t> encode(std::unique_ptr<const FlightProxy::Core::MspPacket> packet) override
                {
                    std::vector<uint8_t> buffer;
                    buffer.push_back('$');
                    buffer.push_back('X');
                    buffer.push_back(packet->direction); // '<' o '>'
                    buffer.push_back(0);                 // Flag (siempre 0)

                    uint16_t cmd = packet->command;
                    uint16_t payloadSize = static_cast<uint16_t>(packet->payload.size());

                    // Command (Little-Endian)
                    buffer.push_back(static_cast<uint8_t>(cmd & 0xFF));
                    buffer.push_back(static_cast<uint8_t>((cmd >> 8) & 0xFF));

                    // Payload Size (Little-Endian)
                    buffer.push_back(static_cast<uint8_t>(payloadSize & 0xFF));
                    buffer.push_back(static_cast<uint8_t>((payloadSize >> 8) & 0xFF));

                    // Payload
                    buffer.insert(buffer.end(), packet->payload.begin(), packet->payload.end());

                    // Calcular Checksum (CRC8 DVB-S2)
                    // Se calcula sobre (Flag, Cmd, Size, Payload)
                    uint8_t checksum = 0;
                    for (size_t i = 3; i < buffer.size(); ++i)
                    {
                        checksum = Detail::crc8_dvb_s2(checksum, buffer[i]);
                    }
                    buffer.push_back(checksum);

                    return buffer;
                }
            };

            class MspDecoder : public IDecoderT<FlightProxy::Core::MspPacket>
            {
            private:
                enum class ParseState
                {
                    IDLE,
                    HEADER_X,
                    DIRECTION,
                    FLAG,
                    CMD_L,
                    CMD_H,
                    SIZE_L,
                    SIZE_H,
                    PAYLOAD,
                    CHECKSUM
                };

                ParseState state_ = ParseState::IDLE;
                FlightProxy::Core::MspPacket workingPacket_;
                uint16_t payloadSize_ = 0;
                uint16_t payloadCounter_ = 0;
                uint16_t tempCmd_ = 0;
                uint16_t tempSize_ = 0;
                uint8_t calculatedChecksum_ = 0;
                std::function<void(std::unique_ptr<const FlightProxy::Core::MspPacket>)> onPacketHandler_;

                // Procesa un solo byte
                void parse(uint8_t byte)
                {
                    switch (state_)
                    {
                    case ParseState::IDLE:
                        if (byte == '$')
                        {
                            // FP_LOG_D("MspDecoder", "Inicio de paquete MSP detectado");
                            reset(); // Iniciar un paquete nuevo y limpio
                            state_ = ParseState::HEADER_X;
                        }
                        break;

                    case ParseState::HEADER_X:
                        // FP_LOG_D("MspDecoder", "Cabecera 'X' de MSP detectada");
                        state_ = (byte == 'X') ? ParseState::DIRECTION : ParseState::IDLE;
                        break;

                    case ParseState::DIRECTION:
                        if (byte == '<' || byte == '>' || byte == '!')
                        {
                            // FP_LOG_D("MspDecoder", "Dirección de MSP detectada");
                            workingPacket_.direction = byte;
                            calculatedChecksum_ = 0;
                            state_ = ParseState::FLAG;
                        }
                        else
                        {
                            // FP_LOG_W("MspDecoder", "Dirección de MSP inválida: %c", byte);
                            state_ = ParseState::IDLE;
                        }
                        break;

                    case ParseState::FLAG:
                        // El flag es parte del checksum, pero no lo guardamos
                        // FP_LOG_D("MspDecoder", "Flag de MSP procesado");
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);
                        state_ = ParseState::CMD_L;
                        break;

                    case ParseState::CMD_L:
                        // FP_LOG_D("MspDecoder", "ComandoL de MSP procesado");
                        tempCmd_ = byte;
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);
                        state_ = ParseState::CMD_H;
                        break;

                    case ParseState::CMD_H:
                        // FP_LOG_D("MspDecoder", "ComandoH de MSP procesado");
                        tempCmd_ |= (static_cast<uint16_t>(byte) << 8);
                        workingPacket_.command = tempCmd_;
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);
                        state_ = ParseState::SIZE_L;
                        break;

                    case ParseState::SIZE_L:
                        // FP_LOG_D("MspDecoder", "TamañoL de MSP procesado");
                        tempSize_ = byte;
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);
                        state_ = ParseState::SIZE_H;
                        break;

                    case ParseState::SIZE_H:
                        // FP_LOG_D("MspDecoder", "TamañoH de MSP procesado");
                        tempSize_ |= (static_cast<uint16_t>(byte) << 8);
                        payloadSize_ = tempSize_;
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);

                        if (payloadSize_ > 0)
                        {
                            workingPacket_.payload.reserve(payloadSize_);
                            payloadCounter_ = 0;
                            state_ = ParseState::PAYLOAD;
                        }
                        else
                        {
                            state_ = ParseState::CHECKSUM; // Sin payload
                        }
                        break;

                    case ParseState::PAYLOAD:
                        // FP_LOG_D("MspDecoder", "Payload de MSP procesado");
                        workingPacket_.payload.push_back(byte);
                        calculatedChecksum_ = Detail::crc8_dvb_s2(calculatedChecksum_, byte);
                        payloadCounter_++;
                        if (payloadCounter_ == payloadSize_)
                        {
                            state_ = ParseState::CHECKSUM;
                        }
                        break;

                    case ParseState::CHECKSUM:
                        // FP_LOG_D("MspDecoder", "Checksum de MSP procesado recibido: 0x%02X, calculado: 0x%02X", byte, calculatedChecksum_);
                        if (byte == calculatedChecksum_)
                        {
                            // FP_LOG_D("MspDecoder", "Checksum correcto");
                            if (onPacketHandler_)
                            {
                                // FP_LOG_D("MspDecoder", "Llamando al handler de paquete MSP");
                                onPacketHandler_(std::make_unique<FlightProxy::Core::MspPacket>(workingPacket_));
                            }
                        }
                        // Siempre resetear, haya sido bueno o malo el checksum
                        reset();
                        break;
                    }
                }

            public:
                MspDecoder()
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

                void onPacket(std::function<void(std::unique_ptr<const FlightProxy::Core::MspPacket>)> handler) override
                {
                    onPacketHandler_ = handler;
                }

                // Ahora es público y cumple la interfaz
                void reset() override
                {
                    state_ = ParseState::IDLE;
                    payloadSize_ = 0;
                    payloadCounter_ = 0;
                    calculatedChecksum_ = 0;
                    tempCmd_ = 0;
                    tempSize_ = 0;
                    workingPacket_.payload.clear();
                }
            };
        }
    }
}