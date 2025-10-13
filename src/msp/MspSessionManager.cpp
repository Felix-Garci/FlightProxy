#include "MspSessionManager.hpp"
#include "utils/MutexGuard.hpp"

using fcbridge::utils::MutexGuard;

namespace fcbridge::msp
{

MspSessionManager::MspSessionManager()
{
    mtx_ = xSemaphoreCreateMutex();
}

MspSessionManager::~MspSessionManager()
{
    if (mtx_)
    {
        vSemaphoreDelete(mtx_);
        mtx_ = nullptr;
    }
}

void MspSessionManager::setCtrlHandler(LocalCtrlHandler *h) { ctrl_ = h; }
void MspSessionManager::setProxy(MspPassthroughProxy *p) { proxy_ = p; }
void MspSessionManager::setTcpEndpoint(net::TcpMspEndpoint *ep) { tcp_ = ep; }

bool MspSessionManager::submit(const MspFrame &req)
{
    // Placeholder lock to illustrate FreeRTOS mutex usage
    MutexGuard lock(mtx_);
    (void)req;
    return false;
}

void MspSessionManager::onFcResponse(const MspFrame &resp)
{
    (void)resp;
}

void MspSessionManager::setTimeouts(uint32_t fcTimeoutMs,
                                    uint32_t localTimeoutMs)
{
    MutexGuard lock(mtx_);
    fcTimeoutTicks_ = pdMS_TO_TICKS(fcTimeoutMs);
    localTimeoutTicks_ = pdMS_TO_TICKS(localTimeoutMs);
}

void MspSessionManager::deliverToClient(const MspFrame &resp)
{
    (void)resp;
}

} // namespace fcbridge::msp
