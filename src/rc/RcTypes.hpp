#pragma once
#include <cstdint>
#include <array>
#include <chrono>

namespace fcbridge::rc
{

    constexpr size_t RC_MAX_CHANNELS = 14;

    using micros = std::chrono::microseconds;
    using millis = std::chrono::milliseconds;

    struct RcSample
    {
        std::array<uint16_t, RC_MAX_CHANNELS> ch{}; // unidades iBUS/PWM (1000–2000 típico)
        uint32_t flags = 0;                         // bits de origen/estado
    };

    struct RcSnapshot
    {
        RcSample current;
        RcSample defaults;
        bool fresh = false;
        millis age{0};
    };

    struct RcLimits
    {
        uint16_t min = 1000;
        uint16_t max = 2000;
        uint16_t mid = 1500;
    };

} // namespace fcbridge::rc
