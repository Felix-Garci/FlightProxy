#include "RcChannels.hpp"

void fcbridge::rc::RcChannels::init(const RcLimits &limits, const RcSample &defaults)
{
    std::lock_guard<std::mutex> lock(mtx_);
    limits_ = limits;
    defaults_ = defaults;
    current_ = defaults;
    lastUpdate_ = std::chrono::steady_clock::now();
}

void fcbridge::rc::RcChannels::setFromUdp(const RcSample &s)
{
    std::lock_guard<std::mutex> lock(mtx_);
    current_ = s;
    lastUpdate_ = std::chrono::steady_clock::now();
}

void fcbridge::rc::RcChannels::setFromLocal(const RcSample &s)
{
    std::lock_guard<std::mutex> lock(mtx_);
    current_ = s;
    lastUpdate_ = std::chrono::steady_clock::now();
}

void fcbridge::rc::RcChannels::setDefault(const RcSample &d)
{
    std::lock_guard<std::mutex> lock(mtx_);
    defaults_ = d;
}

fcbridge::rc::RcSnapshot fcbridge::rc::RcChannels::snapshot() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    fcbridge::rc::RcSnapshot rcsnp;
    rcsnp.current = current_;
    rcsnp.defaults = defaults_;
    rcsnp.age = std::chrono::duration_cast<millis>(std::chrono::steady_clock::now() - lastUpdate_);
    rcsnp.fresh = true;
    return rcsnp;
}

bool fcbridge::rc::RcChannels::isFresh(std::chrono::milliseconds timeout) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto age = std::chrono::duration_cast<millis>(std::chrono::steady_clock::now() - lastUpdate_);
    return age <= timeout;
}

void fcbridge::rc::RcChannels::setLimits(const RcLimits &l)
{
    std::lock_guard<std::mutex> lock(mtx_);
    limits_ = l;
}
