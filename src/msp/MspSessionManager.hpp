#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include "msp/MspFrame.hpp"

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
        MspSessionManager() = default;

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
        void setTimeouts(std::chrono::milliseconds fcTimeout,
                         std::chrono::milliseconds localTimeout);

    private:
        // Entrega al cliente (TCP) la respuesta final
        void deliverToClient(const MspFrame &resp);

        // Estado LOCKSTEP
        std::mutex mtx_;
        bool inFlight_ = false;
        uint8_t inflightCmd_ = 0;
        std::chrono::steady_clock::time_point inflightSince_{};

        // Dependencias
        LocalCtrlHandler *ctrl_ = nullptr;
        MspPassthroughProxy *proxy_ = nullptr;
        net::TcpMspEndpoint *tcp_ = nullptr;

        // Timeouts
        std::chrono::milliseconds fcTimeout_{200};
        std::chrono::milliseconds localTimeout_{50};
    };

} // namespace fcbridge::msp
