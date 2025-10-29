#pragma once

#include "esp_log.h"

/**
 * @brief Envoltorio de macros de log para FlightProxy.
 * * Esto nos da un punto central para controlar todo el logging.
 * Si en el futuro queremos redirigir logs a un archivo o a la red,
 * solo modificamos este archivo.
 * * El '##__VA_ARGS__' es una extensión de C/C++ que maneja correctamente
 * los argumentos variables (incluso si no hay ninguno).
 */

#define FP_LOG_E(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#define FP_LOG_W(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define FP_LOG_I(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define FP_LOG_D(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define FP_LOG_V(tag, format, ...) ESP_LOGV(tag, format, ##__VA_ARGS__)