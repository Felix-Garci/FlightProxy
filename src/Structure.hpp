#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace tp
{
    namespace HAL
    {
        class SimpleTCPServer;
        class SimpleUARTEvt;
        class SimpleUDPReceiver;
        class WiFiSTA;
    }
    namespace UTILS
    {
        class Log;
    }
    namespace MSG
    {
        struct Frame
        {
            uint8_t flag = 0;             // Flag del mensaje
            uint16_t cmd = 0;             // ID MSP
            std::vector<uint8_t> payload; // datos crudos
            uint8_t result = 0;           // código de resultado (solo en respuestas)
        };

        bool decode(const std::vector<uint8_t> &in, Frame &out);
        void encode(const Frame &in, std::vector<uint8_t> &out);
        uint8_t CRC8_DVB_S2(const std::span<const uint8_t> data);

    }
    namespace I
    {
        // Parent
        class Interface;

        // Children
        class Client;
        class FastRC_RX;
        class FC;
        class FastRC_TX;
    }
    namespace T
    {
        // Parent
        class Transaction;

        // Children
        class Client;
        class FC;
    }

}