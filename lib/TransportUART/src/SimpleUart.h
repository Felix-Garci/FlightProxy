#pragma once
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
/*
class SimpleUart{
    -port_: uart_port_t
    -txpin_: gpio_num_t
    -rxpin_: gpio_num_t
    -baudrate_: uint32_t
    -queue_: QueueHandle_t
    -eventTaskHandle_: TaskHandle_t
    -rxBuffer_: uint8_t*
    -rxbuffersize_: size_t

    +SimpleUart(port:uart_port_t,txpin:gpio_num_t,rxpin:gpio_num_t,baudrate:uint32_t)
    +~SimpleUart()
    +open(): void
    +close(): void
    +send(data:const uint8_t*,len:size_t): void
    -eventTask(): void
    -eventTaskAdapter(arg:void*): void
}
*/

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleUart
        {
        public:
            SimpleUart(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate);
            ~SimpleUart();
            void open();
            void close();
            void send(const uint8_t *data, size_t len);

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