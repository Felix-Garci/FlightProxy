#pragma once
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
            class MspEncoder : public IEncoderT<FlightProxy::Core::MspPacket>
            {
            public:
                std::vector<uint8_t> encode(const FlightProxy::Core::MspPacket &packet) override
                {
                    std::vector<uint8_t> buffer;
                    buffer.push_back('$');
                    buffer.push_back('M');
                    buffer.push_back(packet.direction); // '<' o '>'

                    uint8_t payloadSize = static_cast<uint8_t>(packet.payload.size());
                    buffer.push_back(payloadSize);
                    buffer.push_back(packet.command);

                    // Copiar payload
                    buffer.insert(buffer.end(), packet.payload.begin(), packet.payload.end());

                    // Calcular checksum (XOR de size, cmd y payload)
                    uint8_t checksum = 0;
                    checksum ^= payloadSize;
                    checksum ^= packet.command;
                    for (uint8_t byte : packet.payload)
                    {
                        checksum ^= byte;
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
                    HEADER_M,
                    HEADER_DIR,
                    SIZE,
                    COMMAND,
                    PAYLOAD,
                    CHECKSUM
                };

                ParseState state_ = ParseState::IDLE;
                MspPacket currentPacket_;
                uint8_t payloadSize_ = 0;
                uint8_t calculatedChecksum_ = 0;
                std::function<void(const FlightProxy::Core::MspPacket &)> onPacketHandler_;

                void reset()
                {
                    state_ = ParseState::IDLE;
                    currentPacket_.payload.clear();
                    payloadSize_ = 0;
                    calculatedChecksum_ = 0;
                }

                // Procesa un solo byte
                void parse(uint8_t byte)
                {
                    switch (state_)
                    {
                    case ParseState::IDLE:
                        if (byte == '$')
                        {
                            state_ = ParseState::HEADER_M;
                        }
                        break;

                    case ParseState::HEADER_M:
                        state_ = (byte == 'M') ? ParseState::HEADER_DIR : ParseState::IDLE;
                        break;

                    case ParseState::HEADER_DIR:
                        if (byte == '<' || byte == '>')
                        {
                            currentPacket_.direction = byte;
                            state_ = ParseState::SIZE;
                        }
                        else
                        {
                            reset();
                        }
                        break;

                    case ParseState::SIZE:
                        payloadSize_ = byte;
                        currentPacket_.payload.reserve(payloadSize_); // Reservar espacio
                        calculatedChecksum_ = 0;                      // Empezar a calcular checksum
                        calculatedChecksum_ ^= byte;
                        state_ = ParseState::COMMAND;
                        break;

                    case ParseState::COMMAND:
                        currentPacket_.command = byte;
                        calculatedChecksum_ ^= byte;
                        if (payloadSize_ > 0)
                        {
                            state_ = ParseState::PAYLOAD;
                        }
                        else
                        {
                            state_ = ParseState::CHECKSUM; // Ir directo a checksum si no hay payload
                        }
                        break;

                    case ParseState::PAYLOAD:
                        currentPacket_.payload.push_back(byte);
                        calculatedChecksum_ ^= byte;
                        if (currentPacket_.payload.size() == payloadSize_)
                        {
                            state_ = ParseState::CHECKSUM;
                        }
                        break;

                    case ParseState::CHECKSUM:
                        if (byte == calculatedChecksum_)
                        {
                            // ¡Paquete válido!
                            if (onPacketHandler_)
                            {
                                onPacketHandler_(currentPacket_);
                            }
                        }
                        else
                        {
                            // Checksum falló
                        }
                        reset(); // Volver al inicio para el siguiente paquete
                        break;
                    }
                }

            public:
                void feed(const uint8_t *data, size_t len) override
                {
                    for (size_t i = 0; i < len; ++i)
                    {
                        parse(data[i]);
                    }
                }

                void onPacket(std::function<void(const FlightProxy::Core::MspPacket &)> handler) override
                {
                    onPacketHandler_ = handler;
                }
            };
        }
    }
}