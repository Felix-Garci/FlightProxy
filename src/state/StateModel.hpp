#pragma once
#include <cstdint>
#include <mutex>
#include <chrono>

namespace fcbridge::state
{

    class StateModel
    {
    public:
        // Getters
        bool udpEnabled() const;
        uint16_t udpTimeoutMs() const; // frescura RC por UDP
        uint16_t ibusPeriodMs() const;

        // Setters
        void setUdpEnabled(bool on);
        void setUdpTimeoutMs(uint16_t ms);
        void setIbusPeriodMs(uint16_t ms);

    private:
        mutable std::mutex mtx_;
        bool udpEnabled_ = true;
        uint16_t udpTimeoutMs_ = 100; // ej. 100 ms
        uint16_t ibusPeriodMs_ = 7;   // ej. ~140 Hz
    };

} // namespace fcbridge::state
