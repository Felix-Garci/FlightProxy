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
        Incomplete,
        Error
    };

    struct MspFrame
    {
        uint8_t flag = 0;             // Flag del mensaje
        uint16_t cmd = 0;             // ID MSP
        bool isRequest = true;        // true = solicitud, false = respuesta
        std::vector<uint8_t> payload; // datos crudos
        uint8_t result = 0;           // código de resultado (solo en respuestas)
    };

    // Utilidades de codificación/decodificación
    ParseStatus decode(std::span<const uint8_t> stream, MspFrame &out);
    void encode(const MspFrame &in, std::vector<uint8_t> &out);
    inline uint8_t CRC8_DVB_S2(const std::span<const uint8_t> data);

    // Helpers comunes
    inline bool isCtrl(uint8_t cmd) { return cmd <= (0xFFFF / 2); } // Tenemos que decidir un threshold

} // namespace fcbridge::msp
