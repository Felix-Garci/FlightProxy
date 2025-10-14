#include "MspFrame.hpp"

fcbridge::msp::ParseStatus fcbridge::msp::decode(std::span<const uint8_t> stream, MspFrame &out)
{
    // "$X>"
    size_t pos = 0;
    if (stream.size() < 8)
    {
        return ParseStatus::Incomplete;
    }
    if (stream[pos++] != '$')
    {
        return ParseStatus::Invalid;
    }
    if (stream[pos++] != 'X')
    {
        return ParseStatus::Invalid;
    }
    if (stream[pos] == '!')
    {
        return ParseStatus::Error;
    }
    if (stream[pos] == '>')
    {
        out.isRequest = true;
    }
    else if (stream[pos] == '<')
    {
        out.isRequest = false;
    }
    else
    {
        return ParseStatus::Invalid;
    }
    pos++;
    out.flag = stream[pos++];
    out.cmd = stream[pos++] | (stream[pos++] << 8);
    uint16_t payload_length = stream[pos++] | (stream[pos++] << 8);
    uint16_t total_length = 8 + payload_length + 1; // header + payload + crc
    if (stream.size() < total_length)
    {
        return ParseStatus::Incomplete;
    }
    out.payload.resize(payload_length);
    for (size_t i = 0; i < payload_length; i++)
    {
        out.payload[i] = stream[pos++];
    }
    uint8_t received_crc = stream[pos];
    uint8_t computed_crc = CRC8_DVB_S2(stream.subspan(3, total_length - 1 - 3));

    if (received_crc != computed_crc)
    {
        return ParseStatus::CrcError;
    }

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
