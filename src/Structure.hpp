#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <array>
#include <functional>

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
        class MutexGuard;
        using TargetFunction = std::function<void(void)>;
        class Timer;
    }
    namespace MSG
    {
        struct Frame
        {
            uint8_t flag = 0;             // Flag del mensaje
            uint16_t cmd = 0;             // ID MSP
            std::vector<uint8_t> payload; // datos crudos
            uint8_t result = 0;           // código de resultado (solo en respuestas)
            bool isRequest = true;        // Indica si es peticion o es contestacion
        };

        // Default, msp protocol
        bool decode(const std::vector<uint8_t> &stream, Frame &out);
        void encode(const Frame &in, std::vector<uint8_t> &out);
        uint8_t CRC8_DVB_S2(const std::span<const uint8_t> data);

        // Just for in and out Ibus
        constexpr uint8_t IBUS_FRAME_LEN = 32;
        constexpr uint8_t IBUS_HEADER_LEN = 2;
        constexpr uint8_t IBUS_NUM_CHANNELS = 14;
        constexpr uint8_t IBUS_TYPE_RC = 0x40;
        constexpr uint16_t IBUS_PAYLOAD_LEN = IBUS_NUM_CHANNELS * 2;

        bool decode_ibus(const std::vector<uint8_t> &stream, Frame &out);
        void encode_ibus(const Frame &in, std::vector<uint8_t> &out);
        uint16_t CRC_ibus(const std::span<const uint8_t> data);

    }
    namespace I
    {
        // Parent
        class Interface;

        // Children
        class Client;
        class FastRC_RX; // recive ibus
        class FC;
        class FastRC_TX; // Trasmite ibus
    }
    namespace T
    {
        using TargetFunction = std::function<void(const tp::MSG::Frame &)>;
        class Transactor;
        class Connector;
    }
    namespace ADMIN
    {

    }
    namespace STORAGE
    {
        struct RCSample
        {
            std::array<uint16_t, tp::MSG::IBUS_NUM_CHANNELS> ch{};
            uint32_t age{0};
        };

        struct FastRCStatusSample
        {
            bool FastRC_RX_ON{false};
            uint16_t IbusPeriodMS{7}; // 140 Hz
        };

        template <typename SampleT>
        class Manager; // Manejador generico de structs ( gestiona seguridad de acceso concurrente )

    }

}