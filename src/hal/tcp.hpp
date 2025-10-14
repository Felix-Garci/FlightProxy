// simple_tcp_server.h
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <functional>
#include <cstring>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close()
#include <lwip/inet.h>

class SimpleTCPServer
{
public:
    using RxCallback = std::function<void(int sock, const uint8_t *data, size_t len)>; // se llama cuando llegan datos
    using ConnCallback = std::function<void(int sock)>;                                // conexión/desconexión

    SimpleTCPServer()
        : listen_sock_(-1), client_sock_(-1), port_(0),
          task_(nullptr) {}

    // Inicia el servidor en 'port'. Crea el socket, bind y listen. No crea el task aún.
    esp_err_t begin(uint16_t port)
    {
        port_ = port;

        listen_sock_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listen_sock_ < 0)
        {
            ESP_LOGE(TAG, "socket() failed: errno=%d", errno);
            return ESP_FAIL;
        }

        int yes = 1;
        ::setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port_);

        if (::bind(listen_sock_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            ESP_LOGE(TAG, "bind() failed: errno=%d", errno);
            ::close(listen_sock_);
            listen_sock_ = -1;
            return ESP_FAIL;
        }

        if (::listen(listen_sock_, 1) < 0)
        {
            ESP_LOGE(TAG, "listen() failed: errno=%d", errno);
            ::close(listen_sock_);
            listen_sock_ = -1;
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "TCP server listening on port %u", port_);
        return ESP_OK;
    }

    // Lanza un task que acepta un cliente y lee. Al desconectarse, acepta de nuevo.
    esp_err_t start(size_t task_stack = 4096, UBaseType_t prio = tskIDLE_PRIORITY + 2)
    {
        if (listen_sock_ < 0)
            return ESP_FAIL;
        if (task_)
            return ESP_OK; // ya está corriendo
        if (xTaskCreate(&SimpleTCPServer::taskTrampoline, "tcp_srv", task_stack, this, prio, &task_) != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate failed");
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    // Enviar a cliente actual (si hay)
    int write(const uint8_t *data, size_t len)
    {
        if (client_sock_ < 0 || !data || len == 0)
            return 0;
        int n = ::send(client_sock_, data, len, 0);
        return (n < 0) ? 0 : n;
    }
    int write(const char *s)
    {
        return s ? write(reinterpret_cast<const uint8_t *>(s), std::strlen(s)) : 0;
    }

    // Callbacks
    void onReceive(RxCallback cb) { rx_cb_ = cb; }
    void onClientConnected(ConnCallback cb) { on_conn_cb_ = cb; }
    void onClientDisconnected(ConnCallback cb) { on_disconn_cb_ = cb; }

    // Cierra todo
    void stop()
    {
        if (task_)
        {
            TaskHandle_t t = task_;
            task_ = nullptr; // marca para que el loop salga
            vTaskDelete(t);  // matamos el task (simple y directo)
        }
        closeClient_();
        if (listen_sock_ >= 0)
        {
            ::close(listen_sock_);
            listen_sock_ = -1;
        }
    }

    ~SimpleTCPServer() { stop(); }

private:
    static constexpr const char *TAG = "SimpleTCPServer";

    static void taskTrampoline(void *arg)
    {
        static_cast<SimpleTCPServer *>(arg)->serverTask_();
    }

    void serverTask_()
    {
        for (;;)
        {
            // Acepta cliente (bloqueante)
            sockaddr_in6 source_addr{};
            socklen_t addr_len = sizeof(source_addr);
            ESP_LOGI(TAG, "Waiting for a client...");
            client_sock_ = ::accept(listen_sock_, reinterpret_cast<sockaddr *>(&source_addr), &addr_len);
            if (client_sock_ < 0)
            {
                ESP_LOGE(TAG, "accept() failed: errno=%d", errno);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            // Opcional: desactiva Nagle para latencia baja
            int one = 1;
            ::setsockopt(client_sock_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

            if (on_conn_cb_)
                on_conn_cb_(client_sock_);
            ESP_LOGI(TAG, "Client connected, sock=%d", client_sock_);

            // Bucle de recepción
            uint8_t buf[1024];
            for (;;)
            {
                int n = ::recv(client_sock_, buf, sizeof(buf), 0);
                if (n > 0)
                {
                    if (rx_cb_)
                        rx_cb_(client_sock_, buf, static_cast<size_t>(n));
                }
                else if (n == 0)
                {
                    // cierre limpio por parte del cliente
                    ESP_LOGI(TAG, "Client closed connection");
                    if (on_disconn_cb_)
                        on_disconn_cb_(client_sock_);
                    closeClient_();
                    break; // vuelve a aceptar
                }
                else
                { // n < 0
                    if (errno == EINTR)
                        continue;
                    ESP_LOGW(TAG, "recv() error: errno=%d", errno);
                    if (on_disconn_cb_)
                        on_disconn_cb_(client_sock_);
                    closeClient_();
                    break; // vuelve a aceptar
                }
            }
        }
    }

    void closeClient_()
    {
        if (client_sock_ >= 0)
        {
            ::shutdown(client_sock_, SHUT_RDWR);
            ::close(client_sock_);
            client_sock_ = -1;
        }
    }

    int listen_sock_;
    int client_sock_;
    uint16_t port_;
    TaskHandle_t task_;

    RxCallback rx_cb_{};
    ConnCallback on_conn_cb_{};
    ConnCallback on_disconn_cb_{};
};
