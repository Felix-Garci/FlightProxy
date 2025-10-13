#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc/RcTypes.hpp"

namespace fcbridge::rc
{
    class RcChannels;
}
namespace fcbridge::state
{
    class StateModel;
}
namespace fcbridge::ibus
{
    class UartPort;
    class IbusFrame;
}

namespace fcbridge::ibus
{

    class IbusTransmitter
    {
    public:
        IbusTransmitter() = default;

        // Wiring
        void setChannelsSource(rc::RcChannels *src);
        void setStateModel(state::StateModel *state);
        void setUartPort(UartPort *port);

        // Ciclo de vida
        bool start(); // crea hilo/tarea periódica
        void stop();

    private:
        void runLoop(); // periodicidad fija (no comparte core con Wi-Fi)
        static void taskTrampoline(void *arg);

        rc::RcChannels *rc_ = nullptr;
        state::StateModel *state_ = nullptr;
        UartPort *uart_ = nullptr;

        volatile bool running_ = false;
        TaskHandle_t task_ = nullptr;
    };

} // namespace fcbridge::ibus
