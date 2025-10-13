#pragma once
#include <cstdint>
#include <mutex>
#include <chrono>
#include "RcTypes.hpp"

namespace fcbridge::rc
{

    class RcChannels
    {
    public:
        RcChannels() = default;

        void init(const RcLimits &limits, const RcSample &defaults);

        // Setters (thread-safe)
        void setFromUdp(const RcSample &s);   // fuente UDP
        void setFromLocal(const RcSample &s); // comandos CTRL
        void setDefault(const RcSample &d);   // reconfiguración de defaults

        // Lecturas
        RcSnapshot snapshot() const;
        bool isFresh(std::chrono::milliseconds timeout) const;

        // Config
        void setLimits(const RcLimits &l);

    private:
        RcSample current_{};
        RcSample defaults_{};
        RcLimits limits_{};
        std::chrono::steady_clock::time_point lastUpdate_{};
        mutable std::mutex mtx_;
    };

} // namespace fcbridge::rc
