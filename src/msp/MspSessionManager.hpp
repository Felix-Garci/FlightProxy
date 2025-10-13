#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <optional>
#include "msp/MspFrame.hpp"
#include "msp/LocalCtrlHandler.hpp"
#include "msp/MspPassthroughProxy.hpp"
#include "net/TcpMspEndpoint.hpp"
#include "utils/MutexGuard.hpp"

namespace fcbridge::msp
{
    class LocalCtrlHandler;
    class MspPassthroughProxy;
}

namespace fcbridge::net
{
    class TcpMspEndpoint;
}

namespace fcbridge::msp
{

    // LOCKSTEP_SINGLE: 1 request en vuelo, timeout y respuesta única
    class MspSessionManager
    {
    public:
        MspSessionManager();
        ~MspSessionManager();

        // Wiring
        void setCtrlHandler(LocalCtrlHandler *h);
        void setProxy(MspPassthroughProxy *p);
        void setTcpEndpoint(net::TcpMspEndpoint *ep);

        // Entrada desde TCP
        // Acepta request si no hay otro en vuelo; bloquea/cola breve opcional
        bool submit(const MspFrame &req);

        // Callback desde Passthrough cuando llega respuesta de la FC
        void onFcResponse(const MspFrame &resp);

        // Config
        void setTimeouts(uint32_t fcTimeoutMs,
                         uint32_t localTimeoutMs);

    private:
        // Entrega al cliente (TCP) la respuesta final
        void deliverToClient(const MspFrame &resp);

        // Estado LOCKSTEP
        SemaphoreHandle_t mtx_ = nullptr;
        bool inFlight_ = false;
        uint16_t inflightCmd_ = 0;
        TickType_t inflightSince_ = 0;

        // Dependencias
        LocalCtrlHandler *ctrl_ = nullptr;
        MspPassthroughProxy *proxy_ = nullptr;
        net::TcpMspEndpoint *tcp_ = nullptr;

        // Timeouts
        TickType_t fcTimeoutTicks_ = pdMS_TO_TICKS(200);
        TickType_t localTimeoutTicks_ = pdMS_TO_TICKS(50);
    };

} // namespace fcbridge::msp
