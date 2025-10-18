// wifi_sta.h
#pragma once
#include "Structure.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include <cstring>
#include <string>

class tp::HAL::WiFiSTA
{
public:
    WiFiSTA() : eg_(nullptr), got_ip_(false) {}

    /**
     * Conecta al Wi-Fi. Si last_octet > 0:
     *   1) Conecta por DHCP y obtiene IP/GW/MASK reales
     *   2) Cambia SOLO el último octeto a 'last_octet' y pone IP estática (misma GW y MASK)
     */
    esp_err_t begin(const char *ssid, const char *pass,
                    uint32_t timeout_ms = 15000,
                    uint8_t last_octet = 0,
                    wifi_auth_mode_t auth = WIFI_AUTH_WPA2_PSK,
                    bool low_latency = true)
    {
        if (!ssid || !pass)
            return ESP_ERR_INVALID_ARG;

        systemInit_();
        if (!netif_)
            netif_ = esp_netif_create_default_wifi_sta();

        // -------- Paso 1: conectar por DHCP --------
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                            &WiFiSTA::eventHandler_, this, &h_wifi_));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                            &WiFiSTA::eventHandler_, this, &h_ip_));

        wifi_config_t wc{};
        strlcpy(reinterpret_cast<char *>(wc.sta.ssid), ssid, sizeof(wc.sta.ssid));
        strlcpy(reinterpret_cast<char *>(wc.sta.password), pass, sizeof(wc.sta.password));
        wc.sta.threshold.authmode = auth;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        wc.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
#endif

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
        if (low_latency)
            esp_wifi_set_ps(WIFI_PS_NONE);

        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_connect());

        if (!eg_)
            eg_ = xEventGroupCreate();
        if (!eg_)
            return ESP_ERR_NO_MEM;

        // Espera IP por DHCP
        EventBits_t bits = xEventGroupWaitBits(eg_, GOT_IP_BIT, pdTRUE, pdTRUE,
                                               pdMS_TO_TICKS(timeout_ms));
        got_ip_ = (bits & GOT_IP_BIT) != 0;
        if (!got_ip_)
            return ESP_ERR_TIMEOUT;

        if (last_octet == 0)
        {
            // DHCP normal, ya estamos conectados
            return ESP_OK;
        }

        // -------- Paso 2: fijar IP estática con mismo GW/MASK y último octeto elegido --------
        // -------- Paso 2: IP estática con mismo GW/MASK y último octeto elegido --------
        esp_netif_ip_info_t ipinfo{};
        ESP_ERROR_CHECK(esp_netif_get_ip_info(netif_, &ipinfo));

        // Convierte a host order para operar por octetos
        uint32_t ip_h = lwip_ntohl(ipinfo.ip.addr);
        uint32_t mask_h = lwip_ntohl(ipinfo.netmask.addr);
        uint32_t gw_h = lwip_ntohl(ipinfo.gw.addr);

        // Verifica /24 y construye nueva IP cambiando solo el último octeto
        bool is_24 = (mask_h == 0xFFFFFF00UL);
        if (!is_24)
        {
            ESP_LOGW(TAG, "Máscara no es /24 (mask=%u.%u.%u.%u); revisa tu política",
                     (unsigned)((mask_h >> 24) & 0xFF),
                     (unsigned)((mask_h >> 16) & 0xFF),
                     (unsigned)((mask_h >> 8) & 0xFF),
                     (unsigned)(mask_h & 0xFF));
        }

        // Conserva los 3 primeros octetos y sustituye el último
        uint32_t new_ip_h = (ip_h & 0xFFFFFF00UL) | last_octet;

        // Extra: valida que GW siga en la misma red
        if (((new_ip_h ^ gw_h) & mask_h) != 0)
        {
            ESP_LOGE(TAG, "La nueva IP no cae en la red del gateway; abortando cambio");
            return ESP_ERR_INVALID_ARG;
        }

        esp_netif_dhcpc_stop(netif_); // detén DHCP antes de fijar IP

        esp_netif_ip_info_t newinfo{};
        newinfo.ip.addr = lwip_htonl(new_ip_h);
        newinfo.netmask.addr = lwip_htonl(mask_h);
        newinfo.gw.addr = lwip_htonl(gw_h);
        ESP_ERROR_CHECK(esp_netif_set_ip_info(netif_, &newinfo));

        // Log bonico
        char ip_before[16], ip_after[16], gw_str[16], mask_str[16];
        snprintf(ip_before, sizeof ip_before, IPSTR, IP2STR(&ipinfo.ip));
        snprintf(ip_after, sizeof ip_after, IPSTR, IP2STR(&newinfo.ip));
        snprintf(gw_str, sizeof gw_str, IPSTR, IP2STR(&newinfo.gw));
        snprintf(mask_str, sizeof mask_str, IPSTR, IP2STR(&newinfo.netmask));
        ESP_LOGI(TAG, "DHCP: %s  ->  Estática: %s  (GW %s, MASK %s)", ip_before, ip_after, gw_str, mask_str);

        return ESP_OK;
    }

    void stop()
    {
        if (h_wifi_)
        {
            esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, h_ip_);
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, h_wifi_);
            h_wifi_ = nullptr;
            h_ip_ = nullptr;
        }
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        if (eg_)
        {
            vEventGroupDelete(eg_);
            eg_ = nullptr;
        }
        got_ip_ = false;
    }

    bool isConnected() const { return got_ip_; }

    std::string ipStr() const
    {
        if (!netif_)
            return {};
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(netif_, &ip) != ESP_OK)
            return {};
        char buf[16];
        snprintf(buf, sizeof(buf), IPSTR, IP2STR(&ip.ip));
        return std::string(buf);
    }

private:
    static esp_err_t systemInit_()
    {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }
        esp_netif_init();
        esp_event_loop_create_default();
        return ESP_OK;
    }

    static void eventHandler_(void *arg, esp_event_base_t base, int32_t id, void *data)
    {
        auto *self = static_cast<WiFiSTA *>(arg);
        if (!self)
            return;
        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
        {
            esp_wifi_connect(); // reconecta simple
        }
        else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
        {
            if (self->eg_)
                xEventGroupSetBits(self->eg_, GOT_IP_BIT);
        }
    }

    static inline esp_netif_t *netif_ = nullptr;
    EventGroupHandle_t eg_;
    esp_event_handler_instance_t h_wifi_{}, h_ip_{};
    bool got_ip_;

    static constexpr EventBits_t GOT_IP_BIT = BIT0;
    static constexpr const char *TAG = "WiFiSTA";

    static void ip4_to_str(uint32_t ip, char *out, size_t n)
    {
        snprintf(out, n, IPSTR, IP2STR(reinterpret_cast<ip4_addr_t *>(&ip)));
    }
};
