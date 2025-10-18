#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <span>
#include <string_view>
#include "msp/MspFrame.hpp"
#include "msp/MspSessionManager.hpp"
#include "utils/Log.hpp"

namespace fcbridge::msp
{
    class MspSessionManager;
}

namespace fcbridge::net
{

    // Endpoint TCP que deframea MSP y entrega/recibe frames
    class TcpMspEndpoint
    {
    public:
        using SendFunc = std::function<bool(std::span<const uint8_t>)>;

        explicit TcpMspEndpoint(msp::MspSessionManager *session);

        // Control
        bool start(); // crea tarea RX/TX y acepta clientes
        void stop();

        // Salida hacia cliente (desde SessionManager)
        bool sendToClient(const msp::MspFrame &resp);

        // Callback interno al recibir bytes TCP
        void onTcpBytes(std::span<const uint8_t> bytes);

        // Para inyectar función real de envío TCP (socket)
        void setLowLevelSender(SendFunc f);

    private:
        void handleIncomingFrame(const msp::MspFrame &f);

        msp::MspSessionManager *session_ = nullptr;
        SendFunc sendRaw_{};
        std::vector<uint8_t> rxBuf_; // para deframing incremental
    };

} // namespace fcbridge::net
