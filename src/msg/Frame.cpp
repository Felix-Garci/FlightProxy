#include "Structure.hpp"

namespace tp::MSG
{
    // raw -> Frame
    bool decode(const std::vector<uint8_t> &stream, Frame &out)
    {
        size_t pos = 0;

        // Cabecera mínima: "$X><flag><cmd lo><cmd hi><len lo><len hi>"
        if (stream.size() < 8)
            return 0;

        if (stream[pos++] != '$')
            return 0;
        if (stream[pos++] != 'X')
            return 0;

        if (stream[pos] == '!')
            return 0;
        if (stream[pos] == '>')
            out.isRequest = true;
        else if (stream[pos] == '<')
            out.isRequest = false;
        else
            return 0;
        pos++;

        // flag
        if (pos >= stream.size())
            return 0;
        out.flag = stream[pos++];

        // cmd (LE)
        if (pos + 1 >= stream.size())
            return 0;
        uint16_t cmd_lo = stream[pos++];
        uint16_t cmd_hi = stream[pos++];
        out.cmd = static_cast<uint16_t>(cmd_lo | (cmd_hi << 8));

        // payload length (LE)
        if (pos + 1 >= stream.size())
            return 0;
        uint16_t len_lo = stream[pos++];
        uint16_t len_hi = stream[pos++];
        uint16_t payload_length = static_cast<uint16_t>(len_lo | (len_hi << 8));

        // tamaño total = 8(header) + payload + 1(CRC)
        uint16_t total_length = static_cast<uint16_t>(8 + payload_length + 1);
        if (stream.size() < total_length)
            return 0;

        // payload
        if (pos + payload_length > stream.size())
            return 0;
        out.payload.resize(payload_length);
        for (size_t i = 0; i < payload_length; ++i)
        {
            out.payload[i] = stream[pos++];
        }

        // CRC
        if (pos >= stream.size())
            return 0;
        uint8_t received_crc = stream[pos];

        // CRC sobre el frame sin los 3 primeros bytes ("$X>" o "$X<")
        std::vector<uint8_t> sub(stream.begin() + 3, stream.begin() + stream.size() - 1 - 3);
        uint8_t computed_crc = CRC8_DVB_S2(sub);

        if (received_crc != computed_crc)
            return 0;

        return 1;
    }

    // Frame -> raw
    void encode(const Frame &in, std::vector<uint8_t> &out)
    {
        out.clear();
        out.push_back('$');
        out.push_back('X');
        out.push_back(in.isRequest ? '>' : '<');
        out.push_back(in.flag);
        out.push_back(in.cmd & 0xFF);
        out.push_back((in.cmd >> 8) & 0xFF);
        uint16_t payload_length = static_cast<uint16_t>(in.payload.size());
        out.push_back(payload_length & 0xFF);
        out.push_back((payload_length >> 8) & 0xFF);
        out.insert(out.end(), in.payload.begin(), in.payload.end());
        uint8_t crc = CRC8_DVB_S2(std::span<const uint8_t>(out).subspan(3));
        out.push_back(crc);
    }

    uint8_t CRC8_DVB_S2(const std::span<const uint8_t> data)
    {
        uint8_t crc = 0;
        for (uint8_t b : data)
        {
            crc ^= b;
            for (int i = 0; i < 8; ++i)
            {
                if (crc & 0x80)
                    crc = ((crc << 1) ^ 0xD5) & 0xFF;
                else
                    crc = (crc << 1) & 0xFF;
            }
        }
        return crc;
    }

    // raw -> Frame
    bool decode_ibus(const std::vector<uint8_t> &stream, Frame &out)
    {
        // Validaciones básicas
        if (stream.size() < IBUS_FRAME_LEN) return false;
        if (stream[0] != IBUS_FRAME_LEN)    return false;       
        if (stream[1] != IBUS_TYPE_RC)      return false;

        // Verificar checksum
        const uint16_t crc_calc = CRC_ibus(std::span<const uint8_t>(stream.data(), stream.size()));
        const uint16_t crc_recv = static_cast<uint16_t>(stream[IBUS_FRAME_LEN - 2] |
                                                       (static_cast<uint16_t>(stream[IBUS_FRAME_LEN - 1]) << 8));
        if (crc_calc != crc_recv) return false;

        // Extraer payload de canales (bytes 2..29)
        out.flag      = 0;                    
        out.cmd       = 0;
        out.result    = 0;
        out.isRequest = true;

        out.payload.resize(IBUS_PAYLOAD_LEN);
        for (uint8_t i = 0; i < IBUS_PAYLOAD_LEN; ++i) {
            out.payload[i] = stream[IBUS_HEADER_LEN + i];
        }

        return true;
    }

    // Frame -> raw
    void encode_ibus(const Frame &in, std::vector<uint8_t> &out)
    {
        // Validar que sea el comando adecuado y payload de 28 bytes
        if (in.cmd != 0 || in.payload.size() != IBUS_PAYLOAD_LEN) {
            out.clear();
            return;
        }

        out.resize(IBUS_FRAME_LEN);

        // Cabecera
        out[0] = IBUS_FRAME_LEN;
        out[1] = IBUS_TYPE_RC;

        // Copia del payload (28 bytes) SIN memcpy
        for (uint8_t i = 0; i < IBUS_PAYLOAD_LEN; ++i) {
            out[IBUS_HEADER_LEN + i] = in.payload[i];
        }

        // Calcular y escribir checksum (LE)
        const uint16_t crc = CRC_ibus(std::span<const uint8_t>(out.data(), out.size()));
        out[IBUS_FRAME_LEN - 2] = static_cast<uint8_t>(crc & 0xFF);
        out[IBUS_FRAME_LEN - 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    }
    
    uint16_t CRC_ibus(const std::span<const uint8_t> data)
    {
        uint32_t sum = 0;
        const size_t count = (data.size() >= IBUS_FRAME_LEN) ? (IBUS_FRAME_LEN - 2) : data.size();
        for (size_t i = 0; i < count; ++i) sum += data[i];
        return static_cast<uint16_t>(0xFFFFu - (sum & 0xFFFFu));
    }
}
