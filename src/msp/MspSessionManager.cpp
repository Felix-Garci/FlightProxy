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
        MutexGuard lock(mtx_);
        if (!inFlight_)
        {
            inFlight_ = true;
            inflightCmd_ = req.cmd & 0xFFFF;
            inflightSince_ = xTaskGetTickCount();

            if (fcbridge::msp::isCtrl(inflightCmd_))
            {
                ctrl_->handle(req);
                return true;
            }
            else
            {
                proxy_->sendToFc(req);
                return true;
            }
        }
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
