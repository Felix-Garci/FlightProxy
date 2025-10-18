// simple_udp_receiver.h
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cstring>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class SimpleUDPReceiver
{
public:
    using RxCallback = std::function<void(const uint8_t *data, size_t len,
                                          const sockaddr_in &from)>;

    SimpleUDPReceiver() : sock_(-1), port_(0), task_(nullptr) {}
    ~SimpleUDPReceiver() { stop(); }

    /**
     * @brief Inicia el receptor UDP.
     * @param port Puerto UDP local
     * @param cb   Callback para manejar los datos recibidos
     */
    esp_err_t begin(uint16_t port)
    {
        port_ = port;

        sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ < 0)
        {
            ESP_LOGE(TAG, "socket() falló: errno=%d", errno);
            return ESP_FAIL;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (::bind(sock_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            ESP_LOGE(TAG, "bind() falló: errno=%d", errno);
            ::close(sock_);
            sock_ = -1;
            return ESP_FAIL;
        }

        BaseType_t ok = xTaskCreate(&SimpleUDPReceiver::taskTrampoline, "udp_rx",
                                    4096, this, tskIDLE_PRIORITY + 2, &task_);
        if (ok != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate falló");
            ::close(sock_);
            sock_ = -1;
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Esperando UDP en puerto %u", port_);
        return ESP_OK;
    }

    void stop()
    {
        if (task_)
        {
            vTaskDelete(task_);
            task_ = nullptr;
        }
        if (sock_ >= 0)
        {
            ::close(sock_);
            sock_ = -1;
        }
    }

    void onReceive(RxCallback c) { cb_ = c; }

private:
    static constexpr const char *TAG = "SimpleUDPReceiver";

    static void taskTrampoline(void *arg)
    {
        static_cast<SimpleUDPReceiver *>(arg)->recvTask();
    }

    void recvTask()
    {
        uint8_t buf[1024];
        sockaddr_in from{};
        socklen_t fromlen = sizeof(from);

        while (true)
        {
            int n = ::recvfrom(sock_, buf, sizeof(buf), 0,
                               reinterpret_cast<sockaddr *>(&from), &fromlen);
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;
                ESP_LOGW(TAG, "recvfrom error: %d", errno);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            if (n > 0 && cb_)
            {
                cb_(buf, static_cast<size_t>(n), from);
            }
        }
    }

    int sock_;
    uint16_t port_;
    TaskHandle_t task_;
    RxCallback cb_;
};
