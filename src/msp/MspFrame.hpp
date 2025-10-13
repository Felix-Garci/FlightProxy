#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <optional>

namespace fcbridge::msp
{

    // Dirección del frame para enrutar (cliente TCP ↔ FC UART)
    enum class Direction : uint8_t
    {
        ToFC,
        ToClient
    };

    // Resultado de parseo/validación
    enum class ParseStatus : uint8_t
    {
        Ok,
        Invalid,
        CrcError,
        Incomplete
    };

    struct MspFrame
    {
        uint8_t flag = 0;             // Flag del mensaje
        uint8_t cmd = 0;              // ID MSP
        bool isRequest = true;        // true = solicitud, false = respuesta
        std::vector<uint8_t> payload; // datos crudos
        uint8_t result = 0;           // código de resultado (solo en respuestas)
        uint16_t crc = 0;             // CRC8 del frame
    };

    // Utilidades de codificación/decodificación
    ParseStatus decode(std::span<const uint8_t> stream, MspFrame &out, size_t &consumed);
    void encode(const MspFrame &in, std::vector<uint8_t> &out);

    // Helpers comunes
    inline bool isCtrl(uint8_t cmd) { return cmd >= 0xE0; } // Tenemos que decidir un threshold

} // namespace fcbridge::msp
