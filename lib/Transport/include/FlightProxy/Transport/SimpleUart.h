#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/task.h"

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleUart : public ITransport
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
            QueueHandle_t queue_;
            TaskHandle_t eventTaskHandle_;
            uint8_t *rxBuffer_;
            size_t rxbuffersize_;
            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}