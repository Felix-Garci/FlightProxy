#include "StateModel.hpp"
using fcbridge::utils::MutexGuard;

fcbridge::state::StateModel::StateModel()
{
    mtx_ = xSemaphoreCreateMutex();
}

fcbridge::state::StateModel::~StateModel()
{
    if (mtx_)
    {
        vSemaphoreDelete(mtx_);
        mtx_ = nullptr;
    }
}

bool fcbridge::state::StateModel::udpEnabled() const
{
    MutexGuard lock(mtx_);
    return udpEnabled_;
}

uint16_t fcbridge::state::StateModel::udpTimeoutMs() const
{
    MutexGuard lock(mtx_);
    return udpTimeoutMs_;
}

uint16_t fcbridge::state::StateModel::ibusPeriodMs() const
{

    MutexGuard lock(mtx_);
    return ibusPeriodMs_;
}

void fcbridge::state::StateModel::setUdpEnabled(bool on)
{
    MutexGuard lock(mtx_);
    udpEnabled_ = on;
}

void fcbridge::state::StateModel::setUdpTimeoutMs(uint16_t ms)
{
    MutexGuard lock(mtx_);
    udpTimeoutMs_ = ms;
}

void fcbridge::state::StateModel::setIbusPeriodMs(uint16_t ms)
{
    MutexGuard lock(mtx_);
    ibusPeriodMs_ = ms;
}
