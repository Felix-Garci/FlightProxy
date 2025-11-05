#pragma once

#include <string>
#include "esp_wifi.h"
#include "esp_event.h"

namespace FlightProxy
{
    namespace Connectivity
    {
        class WiFiManager
        {
        public:
            /**
             * @brief Constructor
             */
            WiFiManager();

            /**
             * @brief Destructor
             */
            ~WiFiManager();

            /**
             * @brief Inicializa y conecta a la red WiFi especificada.
             * Esta función es bloqueante.
             *
             * @param ssid El SSID de la red.
             * @param password La contraseña de la red.
             * @return true si la conexión fue exitosa, false en caso contrario.
             */
            bool connect(const std::string &ssid, const std::string &password);

            /**
             * @brief Desconecta y desinicializa la WiFi.
             */
            void disconnect();

        private:
            /**
             * @brief Handler estático para los eventos de WiFi y IP.
             * Se usa como callback de C para el sistema de eventos de ESP-IDF.
             */
            static void event_handler(void *arg, esp_event_base_t event_base,
                                      int32_t event_id, void *event_data);

            // Grupo de eventos de FreeRTOS para sincronizar el estado de la conexión
            EventGroupHandle_t wifi_event_group;

            // Contandor de reintentos
            int retry_count;

            // Instancias de los handlers para poder desregistrarlos
            esp_event_handler_instance_t instance_any_id;
            esp_event_handler_instance_t instance_got_ip;

            // Interfaz de red (netif) para la estación WiFi
            esp_netif_t *sta_netif;

            // Máximo de reintentos de conexión
            static const int MAX_RETRY = 5;

            // Bits para el grupo de eventos
            static const int WIFI_CONNECTED_BIT = BIT0;
            static const int WIFI_FAIL_BIT = BIT1;
        };
    }
}