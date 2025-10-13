#include "StateModel.hpp"

bool fcbridge::state::StateModel::udpEnabled() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return udpEnabled_;
}

uint16_t fcbridge::state::StateModel::udpTimeoutMs() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return udpTimeoutMs_;
}

uint16_t fcbridge::state::StateModel::ibusPeriodMs() const
{

    std::lock_guard<std::mutex> lock(mtx_);
    return ibusPeriodMs_;
}

void fcbridge::state::StateModel::setUdpEnabled(bool on)
{
    std::lock_guard<std::mutex> lock(mtx_);
    udpEnabled_ = on;
}

void fcbridge::state::StateModel::setUdpTimeoutMs(uint16_t ms)
{
    std::lock_guard<std::mutex> lock(mtx_);
    udpTimeoutMs_ = ms;
}

void fcbridge::state::StateModel::setIbusPeriodMs(uint16_t ms)
{
    std::lock_guard<std::mutex> lock(mtx_);
    ibusPeriodMs_ = ms;
}
