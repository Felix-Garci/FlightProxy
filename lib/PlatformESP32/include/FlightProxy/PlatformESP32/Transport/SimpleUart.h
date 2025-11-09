#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#include <memory>

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Transport
        {
            class SimpleUart : public FlightProxy::Core::Transport::ITransport,
                               public std::enable_shared_from_this<SimpleUart>
            {
            public:
                SimpleUart(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate);
                ~SimpleUart() override;

                void open() override;
                void close() override;
                void send(const uint8_t *data, size_t len) override;

            private:
                uart_port_t port_;
                gpio_num_t txpin_;
                gpio_num_t rxpin_;
                uint32_t baudrate_;

                TaskHandle_t eventTaskHandle_;
                QueueHandle_t queue_;
                uint8_t *rxBuffer_;
                size_t rxbuffersize_;
                SemaphoreHandle_t mutex_;

                void eventTask(std::shared_ptr<SimpleUart> *self_ptr_on_heap);
                static void eventTaskAdapter(void *arg);
            };
        }
    }
}