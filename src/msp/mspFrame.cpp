#include "MspFrame.hpp"

fcbridge::msp::ParseStatus fcbridge::msp::decode(std::span<const uint8_t> stream, MspFrame &out)
{
    size_t pos = 0;

    // Cabecera mínima: "$X><flag><cmd lo><cmd hi><len lo><len hi>"
    if (stream.size() < 8)
        return ParseStatus::Incomplete;

    if (stream[pos++] != '$')
        return ParseStatus::Invalid;
    if (stream[pos++] != 'X')
        return ParseStatus::Invalid;

    if (stream[pos] == '!')
        return ParseStatus::Error;
    if (stream[pos] == '>')
        out.isRequest = true;
    else if (stream[pos] == '<')
        out.isRequest = false;
    else
        return ParseStatus::Invalid;
    pos++;

    // flag
    if (pos >= stream.size())
        return ParseStatus::Incomplete;
    out.flag = stream[pos++];

    // cmd (LE)
    if (pos + 1 >= stream.size())
        return ParseStatus::Incomplete;
    uint16_t cmd_lo = stream[pos++];
    uint16_t cmd_hi = stream[pos++];
    out.cmd = static_cast<uint16_t>(cmd_lo | (cmd_hi << 8));

    // payload length (LE)
    if (pos + 1 >= stream.size())
        return ParseStatus::Incomplete;
    uint16_t len_lo = stream[pos++];
    uint16_t len_hi = stream[pos++];
    uint16_t payload_length = static_cast<uint16_t>(len_lo | (len_hi << 8));

    // tamaño total = 8(header) + payload + 1(CRC)
    uint16_t total_length = static_cast<uint16_t>(8 + payload_length + 1);
    if (stream.size() < total_length)
        return ParseStatus::Incomplete;

    // payload
    if (pos + payload_length > stream.size())
        return ParseStatus::Incomplete;
    out.payload.resize(payload_length);
    for (size_t i = 0; i < payload_length; ++i)
    {
        out.payload[i] = stream[pos++];
    }

    // CRC
    if (pos >= stream.size())
        return ParseStatus::Incomplete;
    uint8_t received_crc = stream[pos];

    // CRC sobre el frame sin los 3 primeros bytes ("$X>" o "$X<")
    uint8_t computed_crc = CRC8_DVB_S2(stream.subspan(3, total_length - 1 - 3));

    if (received_crc != computed_crc)
        return ParseStatus::CrcError;

    return ParseStatus::Ok;
}

void fcbridge::msp::encode(const MspFrame &in, std::vector<uint8_t> &out)
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

uint8_t fcbridge::msp::CRC8_DVB_S2(const std::span<const uint8_t> data)
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
