#pragma once
#include <cstdint>
#include <optional>
#include "msp/MspFrame.hpp"
#include "rc/RcTypes.hpp"

namespace fcbridge::rc
{
    class RcChannels;
}

namespace fcbridge::state
{
    class StateModel;
}

namespace fcbridge::msp
{

    // Interpreta MSP CTRL (0xE0–0xFF) y devuelve siempre respuesta
    class LocalCtrlHandler
    {
    public:
        void setRc(rc::RcChannels *rc);
        void setState(state::StateModel *st);

        // Entrada: request de control; Salida: respuesta MSP (ACK/STATUS/DATA)
        MspFrame handle(const MspFrame &ctrlReq);

        // API auxiliar expuesta por contratos
        void setRcDefault(const rc::RcSample &s);
        void setRcValue(const rc::RcSample &s);
        MspFrame getRcStatus();

        void setUdpEnabled(bool on);
        void setTimeoutMs(uint16_t ms);
        void setIbusPeriodMs(uint16_t ms);

    private:
        rc::RcChannels *rc_ = nullptr;
        state::StateModel *st_ = nullptr;

        // Helpers internos para mapear comandos a acciones
        MspFrame makeAck(uint8_t cmd);
        MspFrame makeError(uint8_t cmd, uint8_t code);
        MspFrame makeStatusPayload(uint8_t cmd, const rc::RcSnapshot &snap);
    };

} // namespace fcbridge::msp
