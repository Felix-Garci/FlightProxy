#include "IbusTransmitter.hpp"
#include "IbusFrame.hpp"
#include "IbusUartPort.hpp"
#include "rc/RcChannels.hpp"
#include "state/StateModel.hpp"
#include <array>
#include <span>

void fcbridge::ibus::IbusTransmitter::setChannelsSource(rc::RcChannels *src)
{
    rc_ = src;
}

void fcbridge::ibus::IbusTransmitter::setStateModel(state::StateModel *state)
{
    state_ = state;
}

void fcbridge::ibus::IbusTransmitter::setUartPort(UartPort *port)
{
    uart_ = port;
}

bool fcbridge::ibus::IbusTransmitter::start()
{
    if (!rc_ || !uart_ || !state_)
        return false;

    if (task_ != nullptr)
        return true;

    running_ = true;

    BaseType_t ok = xTaskCreate(
        &IbusTransmitter::taskTrampoline,
        "ibus_tx",
        4096,
        this,
        tskIDLE_PRIORITY + 3,
        &task_);
    return ok == pdPASS;
}

void fcbridge::ibus::IbusTransmitter::stop()
{
    running_ = false;
    // Wait for task to exit and clear handle
    while (task_ != nullptr)
    {
        vTaskDelay(1);
    }
}

void fcbridge::ibus::IbusTransmitter::runLoop()
{
    // Fixed-rate periodic transmission using vTaskDelayUntil
    TickType_t lastWake = xTaskGetTickCount();
    while (running_)
    {
        const uint16_t period_ms = state_->ibusPeriodMs();
        // const uint16_t udp_timeout_ms = state_->udpTimeoutMs();

        // Select current or defaults based on freshness
        auto src = rc_->snapshot().current;

        std::array<uint16_t, ibus::IbusFrame::NUM_CHANNELS> ch{};
        for (size_t i = 0; i < ch.size(); ++i)
        {
            ch[i] = src.ch[i];
        }

        std::array<uint8_t, ibus::IbusFrame::FRAME_LEN> buf{};
        IbusFrame::encode(std::span<const uint16_t, IbusFrame::NUM_CHANNELS>(ch),
                          std::span<uint8_t, IbusFrame::FRAME_LEN>(buf));
        uart_->write(std::span<const uint8_t>(buf));
        uart_->flush();

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(period_ms));
    }
}

void fcbridge::ibus::IbusTransmitter::taskTrampoline(void *arg)
{
    auto *self = static_cast<IbusTransmitter *>(arg);
    self->runLoop();
    // Signal stop completion and delete self task
    self->task_ = nullptr;
    vTaskDelete(nullptr);
}
