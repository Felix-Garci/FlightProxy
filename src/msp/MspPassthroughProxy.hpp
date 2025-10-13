#pragma once
#include <cstdint>
#include <functional>
#include <atomic>
#include <thread>
#include "msp/MspFrame.hpp"
#include "msp/MspSessionManager.hpp"

namespace fcbridge::msp
{
    class MspSessionManager;
}

namespace fcbridge::msp
{

    // Envía MSP a la FC por UART y notifica la siguiente respuesta
    class MspPassthroughProxy
    {
    public:
        explicit MspPassthroughProxy(MspSessionManager *session);

        // Enviar request hacia la FC (LOCKSTEP: una en vuelo)
        bool sendToFc(const MspFrame &req);

        // Callback desde driver UART cuando llega una respuesta completa MSP
        void onUartBytes(std::span<const uint8_t> bytes);

        // Wiring bajo nivel (función real de envío UART)
        using UartWrite = std::function<bool(std::span<const uint8_t>)>;
        void setUartWriter(UartWrite w);

    private:
        void notifySession(const MspFrame &resp);

        MspSessionManager *session_ = nullptr;
        UartWrite write_{};
        std::vector<uint8_t> rxBuf_;
    };

} // namespace fcbridge::msp
