#include "FlightProxy/Connectivity/WifiManager.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <cstring>

namespace FlightProxy
{
    namespace Connectivity
    {
        static const char *TAG = "WiFiManager";

        WiFiManager::WiFiManager() : wifi_event_group(nullptr),
                                     retry_count(0),
                                     instance_any_id(nullptr),
                                     instance_got_ip(nullptr),
                                     sta_netif(nullptr)
        {
        }

        WiFiManager::~WiFiManager()
        {
            // Asegurarse de que todo esté limpio si el objeto se destruye
            disconnect();
            if (wifi_event_group)
            {
                vEventGroupDelete(wifi_event_group);
            }
        }

        void WiFiManager::event_handler(void *arg, esp_event_base_t event_base,
                                        int32_t event_id, void *event_data)
        {
            // 'arg' es el puntero 'this' de nuestra instancia de WiFiManager
            WiFiManager *manager = static_cast<WiFiManager *>(arg);

            if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
            {
                // El driver de WiFi está listo, podemos intentar conectar
                FP_LOG_I(TAG, "WiFi Station iniciada, conectando...");
                esp_wifi_connect();
            }
            else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
            {
                // Se perdió la conexión
                if (manager->retry_count < MAX_RETRY)
                {
                    manager->retry_count++;
                    esp_wifi_connect();
                    FP_LOG_I(TAG, "Reintentando conexión (intento %d/%d)...",
                             manager->retry_count, MAX_RETRY);
                }
                else
                {
                    FP_LOG_E(TAG, "Fallo al conectar después de %d intentos.", MAX_RETRY);
                    // Informar al hilo principal que la conexión falló
                    xEventGroupSetBits(manager->wifi_event_group, WIFI_FAIL_BIT);
                }
            }
            else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
            {
                // ¡Conexión exitosa! Obtuvimos una IP
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                FP_LOG_I(TAG, "¡Conectado! IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
                manager->retry_count = 0; // Reiniciar contador de reintentos
                // Informar al hilo principal que la conexión fue exitosa
                xEventGroupSetBits(manager->wifi_event_group, WIFI_CONNECTED_BIT);
            }
        }

        bool WiFiManager::connect(const std::string &ssid, const std::string &password)
        {
            FP_LOG_I(TAG, "Iniciando conexión a %s...", ssid.c_str());

            // --- 1. Inicialización del sistema base ---
            // (Estas funciones son seguras de llamar múltiples veces)

            // Inicializar NVS (Non-Volatile Storage)
            esp_err_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
            }
            ESP_ERROR_CHECK(ret);

            // Inicializar el stack TCP/IP
            ESP_ERROR_CHECK(esp_netif_init());

            // Crear el bucle de eventos por defecto
            ESP_ERROR_CHECK(esp_event_loop_create_default());

            // --- 2. Configuración de WiFi ---

            // Crear el grupo de eventos para esperar la conexión
            wifi_event_group = xEventGroupCreate();

            // Crear la interfaz de red (netif) para el modo Estación (STA)
            sta_netif = esp_netif_create_default_wifi_sta();

            // Inicializar la WiFi con la configuración por defecto
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            // --- 3. Registro de Event Handlers ---
            // Nos registramos para recibir eventos de WiFi y de IP
            ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                                ESP_EVENT_ANY_ID,
                                                                &event_handler,
                                                                this, // Pasamos 'this' como argumento
                                                                &instance_any_id));
            ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                                IP_EVENT_STA_GOT_IP,
                                                                &event_handler,
                                                                this, // Pasamos 'this' como argumento
                                                                &instance_got_ip));

            // --- 4. Configuración de SSID y Contraseña ---
            wifi_config_t wifi_config = {};
            strncpy((char *)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
            strncpy((char *)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password) - 1);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

            // --- 5. Iniciar y Conectar ---
            retry_count = 0; // Reiniciar contador de reintentos
            ESP_ERROR_CHECK(esp_wifi_start());

            // --- 6. Esperar a la Conexión ---
            // Esta es la parte bloqueante. Esperamos por el bit de CONECTADO o el de FALLO.
            EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                   pdFALSE,        // No limpiar los bits al salir
                                                   pdFALSE,        // No esperar por ambos bits (OR lógico)
                                                   portMAX_DELAY); // Esperar indefinidamente

            // --- 7. Resultado y Limpieza ---
            bool success = false;
            if (bits & WIFI_CONNECTED_BIT)
            {
                FP_LOG_I(TAG, "Conexión exitosa a %s.", ssid.c_str());
                success = true;
            }
            else if (bits & WIFI_FAIL_BIT)
            {
                FP_LOG_E(TAG, "Fallo al conectar a %s.", ssid.c_str());
                success = false;
            }
            else
            {
                FP_LOG_E(TAG, "Error inesperado al esperar la conexión.");
                success = false;
            }

            // Desregistrar los handlers de eventos
            ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
            ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
            instance_got_ip = nullptr;
            instance_any_id = nullptr;

            // Eliminar el grupo de eventos
            vEventGroupDelete(wifi_event_group);
            wifi_event_group = nullptr;

            // Si fallamos, detenemos la WiFi para limpiar
            if (!success)
            {
                esp_wifi_stop();
            }

            return success;
        }

        void WiFiManager::disconnect()
        {
            if (sta_netif == nullptr)
            {
                // Ya está desconectado o nunca se conectó
                return;
            }

            FP_LOG_I(TAG, "Desconectando y desinicializando WiFi...");

            // Asegurarse de que los handlers están desregistrados
            if (instance_got_ip)
            {
                esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
                instance_got_ip = nullptr;
            }
            if (instance_any_id)
            {
                esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
                instance_any_id = nullptr;
            }

            // Detener la WiFi
            ESP_ERROR_CHECK(esp_wifi_stop());

            // Desinicializar el driver WiFi
            ESP_ERROR_CHECK(esp_wifi_deinit());

            // Destruir la interfaz de red
            esp_netif_destroy(sta_netif);
            sta_netif = nullptr;

            // Nota: No desinicializamos nvs, netif_init ni el bucle de eventos,
            // ya que otros módulos podrían estar usándolos.
        }
    }

}