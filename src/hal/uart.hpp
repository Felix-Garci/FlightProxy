// simple_uart_evt.h
#pragma once
#include "esp_check.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <functional>
#include <cstring>

class SimpleUARTEvt
{
public:
    using RxCallback = std::function<void(const uint8_t *, size_t)>;

    SimpleUARTEvt() : port_(UART_NUM_MAX), uart_queue_(nullptr), task_(nullptr) {}

    esp_err_t begin(uart_port_t port, gpio_num_t tx, gpio_num_t rx,
                    int baud = 115200, size_t bufsize = 1024,
                    int queue_len = 10, RxCallback cb = nullptr)
    {
        port_ = port;
        cb_ = cb;

        uart_config_t cfg = {
            .baud_rate = baud,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_DEFAULT
#endif
        };

        ESP_RETURN_ON_ERROR(uart_param_config(port_, &cfg), TAG, "param");
        ESP_RETURN_ON_ERROR(uart_set_pin(port_, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
                            TAG, "pins");

        // Instala driver con cola de eventos
        ESP_RETURN_ON_ERROR(uart_driver_install(port_, bufsize, bufsize, queue_len, &uart_queue_, 0),
                            TAG, "install");

        // Opcional: dispara evento si RX está inactivo ~2 chars o si hay >= 1 char en FIFO
        uart_set_rx_timeout(port_, 2);        // en "periodos" de caracter
        uart_set_rx_full_threshold(port_, 1); // umbral de FIFO

        // Crea el task que procesará los eventos
        BaseType_t ok = xTaskCreate(&SimpleUARTEvt::taskTrampoline, "uart_evt",
                                    3 * 1024, this, tskIDLE_PRIORITY + 2, &task_);
        if (ok != pdPASS)
            return ESP_FAIL;

        return ESP_OK;
    }

    int write(const uint8_t *data, size_t len)
    {
        if (port_ == UART_NUM_MAX || !data || len == 0)
            return 0;
        int w = uart_write_bytes(port_, data, len);
        return (w < 0) ? 0 : w;
    }
    int write(const char *s) { return s ? write(reinterpret_cast<const uint8_t *>(s), std::strlen(s)) : 0; }

    void onReceive(RxCallback cb) { cb_ = cb; }

    ~SimpleUARTEvt()
    {
        if (task_)
        {
            vTaskDelete(task_);
            task_ = nullptr;
        }
        if (port_ != UART_NUM_MAX)
        {
            uart_driver_delete(port_);
        }
    }

private:
    static constexpr const char *TAG = "SimpleUARTEvt";

    static void taskTrampoline(void *arg)
    {
        static_cast<SimpleUARTEvt *>(arg)->eventTask();
    }

    void eventTask()
    {
        uart_event_t evt;
        uint8_t tmp[256];

        for (;;)
        {
            if (xQueueReceive(uart_queue_, &evt, portMAX_DELAY) == pdTRUE)
            {
                switch (evt.type)
                {
                case UART_DATA:
                {
                    size_t to_read = 0;
                    uart_get_buffered_data_len(port_, &to_read);
                    while (to_read)
                    {
                        size_t chunk = (to_read > sizeof(tmp)) ? sizeof(tmp) : to_read;
                        int n = uart_read_bytes(port_, tmp, chunk, 0);
                        if (n > 0 && cb_)
                            cb_(tmp, n);
                        to_read -= (n > 0) ? n : 0;
                        if (n <= 0)
                            break;
                    }
                    break;
                }
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    // Limpia para recuperarse de overflow
                    uart_flush_input(port_);
                    xQueueReset(uart_queue_);
                    break;
                case UART_PATTERN_DET:
                    // (opcional si habilitas pattern detect)
                    break;
                default:
                    break;
                }
            }
        }
    }

    uart_port_t port_;
    QueueHandle_t uart_queue_;
    TaskHandle_t task_;
    RxCallback cb_;
};
