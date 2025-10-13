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

void fcbridge::rc::RcChannels::init(const RcLimits &limits, const RcSample &defaults)
{
    MutexGuard lock(mtx_);
    limits_ = limits;
    defaults_ = defaults;
    current_ = defaults;
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::setFromUdp(const RcSample &s)
{
    MutexGuard lock(mtx_);
    current_ = s;
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::setFromLocal(const RcSample &s)
{
    MutexGuard lock(mtx_);
    current_ = s;
    lastTick_ = xTaskGetTickCount();
}

void fcbridge::rc::RcChannels::setDefault(const RcSample &d)
{
    MutexGuard lock(mtx_);
    defaults_ = d;
}

fcbridge::rc::RcSnapshot fcbridge::rc::RcChannels::snapshot() const
{
    MutexGuard lock(mtx_);
    fcbridge::rc::RcSnapshot rcsnp;
    rcsnp.current = current_;
    rcsnp.defaults = defaults_;
    // age in ms from ticks
    rcsnp.age = static_cast<millis>((xTaskGetTickCount() - lastTick_) * portTICK_PERIOD_MS);
    rcsnp.fresh = true;
    return rcsnp;
}

bool fcbridge::rc::RcChannels::isFresh(uint32_t timeout_ms) const
{
    MutexGuard lock(mtx_);
    const TickType_t now = xTaskGetTickCount();
    const TickType_t elapsed = now - lastTick_;
    return elapsed <= pdMS_TO_TICKS(timeout_ms);
}

void fcbridge::rc::RcChannels::setLimits(const RcLimits &l)
{
    MutexGuard lock(mtx_);
    limits_ = l;
}
