#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "HOLA";

#ifndef LED_PIN
#define LED_PIN GPIO_NUM_2 // Cambia si tu placa usa otro LED
#endif

extern "C" void app_main(void)
{
    // Configurar LED como salida
    gpio_config_t io{};
    io.intr_type = GPIO_INTR_DISABLE;
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << LED_PIN);
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io);

    ESP_LOGI(TAG, "Hola mundo desde ESP-IDF 🤖");
    ESP_LOGI(TAG, "Parpadeando LED en GPIO %d", (int)LED_PIN);

    bool on = false;
    while (true)
    {
        on = !on;
        gpio_set_level(LED_PIN, on ? 1 : 0);
        ESP_LOGI(TAG, "Tick! LED %s", on ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms
    }
}
