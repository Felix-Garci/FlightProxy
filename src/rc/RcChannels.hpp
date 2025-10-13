#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "RcTypes.hpp"
#include "utils/MutexGuard.hpp"

namespace fcbridge::rc
{

    class RcChannels
    {
    public:
        RcChannels();
        ~RcChannels();

        void init(const RcLimits &limits, const RcSample &defaults);

        // Setters (thread-safe)
        void setFromUdp(const RcSample &s);   // fuente UDP
        void setFromLocal(const RcSample &s); // comandos CTRL
        void setDefault(const RcSample &d);   // reconfiguración de defaults

        // Lecturas
        RcSnapshot snapshot() const;
        bool isFresh(uint32_t timeout_ms) const;

        // Config
        void setLimits(const RcLimits &l);

    private:
        RcSample current_{};
        RcSample defaults_{};
        RcLimits limits_{};
        TickType_t lastTick_ = 0;
        mutable SemaphoreHandle_t mtx_ = nullptr;
    };

} // namespace fcbridge::rc
