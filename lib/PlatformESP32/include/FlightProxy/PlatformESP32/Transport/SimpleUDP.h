#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>
#include <mutex>
#include "lwip/sockets.h"

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Transport
        {
            class SimpleUDP : public FlightProxy::Core::Transport::ITransport,
                              public std::enable_shared_from_this<SimpleUDP>
            {
            public:
                /**
                 * @brief Constructor para un "servidor" UDP que escucha en un puerto.
                 * @param port Puerto en el que escuchar.
                 */
                SimpleUDP(uint16_t port);
                ~SimpleUDP() override;

                void open() override;
                void close() override;

                /**
                 * @brief Envía datos al último remitente que nos envió un paquete.
                 * * La interfaz ITransport no especifica un destino, por lo que
                 * este es el comportamiento más lógico para un socket UDP de escucha.
                 */
                void send(const uint8_t *data, size_t len) override;

            private:
                int m_sock = -1;
                uint16_t m_port; // Puerto en el que escuchamos

                // Almacena la dirección del último remitente para el método send()
                struct sockaddr_in m_last_sender_addr;
                socklen_t m_last_sender_len;
                bool m_has_last_sender = false; // Flag para saber si podemos enviar

                TaskHandle_t eventTaskHandle_;
                std::unique_ptr<Core::OSAL::IMutex> mutex_;
                void eventTask(std::shared_ptr<SimpleUDP> *self_keep_alive);
                static void eventTaskAdapter(void *arg);
            };
        }
    }
}