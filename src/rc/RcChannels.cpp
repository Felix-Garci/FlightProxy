#include "RcChannels.hpp"
using fcbridge::utils::MutexGuard;

fcbridge::rc::RcChannels::RcChannels()
{
    mtx_ = xSemaphoreCreateMutex();
}

fcbridge::rc::RcChannels::~RcChannels()
{
    if (mtx_)
    {
        vSemaphoreDelete(mtx_);
        mtx_ = nullptr;
    }
}

void fcbridge::rc::RcChannels::init(const RcSample &defaults)
{
    MutexGuard lock(mtx_);
    defaults_ = defaults;
    current_ = defaults;
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::setFromUdp(const RcSample &s)
{
    MutexGuard lock(mtx_);
    current_ = s;
    current_.flags = 1; // from UDP
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::setFromLocal(const RcSample &s)
{
    MutexGuard lock(mtx_);
    current_ = s;
    current_.flags = 2; // from Local
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::applyDefaults()
{
    MutexGuard lock(mtx_);
    current_ = defaults_;
    lastTick_ = xTaskGetTickCount();
}

fcbridge::rc::RcSnapshot fcbridge::rc::RcChannels::snapshot() const
{
    MutexGuard lock(mtx_);
    fcbridge::rc::RcSnapshot rcsnp;
    rcsnp.current = current_;
    rcsnp.defaults = defaults_;
    // age in ms from ticks
    rcsnp.age = static_cast<millis>((xTaskGetTickCount() - lastTick_) * portTICK_PERIOD_MS);
    return rcsnp;
}
