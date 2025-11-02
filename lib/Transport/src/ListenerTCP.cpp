#include "FlightProxy/Transport/ListenerTCP.h"
#include "FlightProxy/Transport/SimpleTCP.h" // ¡Importante! El listener CREA el SimpleTCP
#include "FlightProxy/Core/Utils/Logger.h"   // Usando tu logger

// Includes de lwIP y ESP-IDF
#include "lwip/sockets.h"
#include "esp_log.h"
#include <memory> // Para std::make_shared

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "ListenerTCP";

        ListenerTCP::ListenerTCP()
        {
        }

        ListenerTCP::~ListenerTCP()
        {
            stopListening();
        }

        bool ListenerTCP::startListening(uint16_t port)
        {
            if (m_server_sock != -1)
            {
                FP_LOG_W(TAG, "El listener ya estaba iniciado.");
                return true;
            }

            // 1. Crear el socket
            m_server_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (m_server_sock < 0)
            {
                FP_LOG_E(TAG, "Error al crear socket: %d", errno);
                return false;
            }

            // 2. Configurar la dirección (escuchar en todas las IPs)
            struct sockaddr_in dest_addr;
            dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(port);

            // 3. Bind
            int err = ::bind(m_server_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err != 0)
            {
                FP_LOG_E(TAG, "Error en bind: %d", errno);
                ::close(m_server_sock);
                m_server_sock = -1;
                return false;
            }

            // 4. Listen
            err = ::listen(m_server_sock, 1); // Cola de 1 (simple)
            if (err != 0)
            {
                FP_LOG_E(TAG, "Error en listen: %d", errno);
                ::close(m_server_sock);
                m_server_sock = -1;
                return false;
            }

            // 5. Crear la tarea de FreeRTOS para el bucle accept()
            BaseType_t result = xTaskCreate(
                listener_task_entry,    // Función estática
                "tcp_listener_task",    // Nombre
                4096,                   // Stack
                this,                   // Argumento (puntero a esta instancia)
                5,                      // Prioridad
                &m_listener_task_handle // Handle
            );

            if (result != pdPASS)
            {
                FP_LOG_E(TAG, "Error al crear la tarea del listener");
                ::close(m_server_sock);
                m_server_sock = -1;
                return false;
            }

            FP_LOG_I(TAG, "Listener iniciado en puerto %d", port);
            return true;
        }

        void ListenerTCP::stopListening()
        {
            if (m_server_sock == -1)
            {
                // Ya está parado
                return;
            }

            FP_LOG_I(TAG, "Parando listener...");

            // Al cerrar el socket del servidor, el 'accept()' bloqueante
            // en la tarea del listener fallará inmediatamente.
            ::close(m_server_sock);
            m_server_sock = -1;

            // La tarea se limpiará y borrará a sí misma.
        }

        // --- Tarea de FreeRTOS ---

        void ListenerTCP::listener_task_entry(void *arg)
        {
            ListenerTCP *listener = static_cast<ListenerTCP *>(arg);
            FP_LOG_I(TAG, "Tarea de Listener iniciada.");

            while (true)
            {
                struct sockaddr_in source_addr; // Para guardar la IP del cliente
                socklen_t addr_len = sizeof(source_addr);

                // accept() es BLOQUEANTE. La tarea se "dormirá" aquí.
                int client_sock = ::accept(listener->m_server_sock, (struct sockaddr *)&source_addr, &addr_len);

                if (client_sock < 0)
                {
                    // Error. Si m_server_sock == -1, es un cierre normal.
                    if (listener->m_server_sock == -1)
                    {
                        FP_LOG_I(TAG, "accept() interrumpido por stopListening(). Saliendo.");
                    }
                    else
                    {
                        FP_LOG_E(TAG, "Error en accept(): %d", errno);
                    }
                    break; // Salir del bucle while(true)
                }

                // --- ¡Cliente Aceptado! ---
                FP_LOG_I(TAG, "Cliente conectado! Socket: %d", client_sock);

                if (listener->onChannelAccepted)
                {
                    // ¡Simplemente pasamos el socket!
                    listener->onChannelAccepted(client_sock);
                }
                else
                {
                    FP_LOG_W(TAG, "Cliente aceptado (socket %d), pero no hay suscriptor en onChannelAccepted!", client_sock);
                    // Si no hay suscriptor, debemos cerrar el socket o habrá una fuga
                    ::close(client_sock);
                }
            }

            // --- Limpieza de la tarea ---
            FP_LOG_I(TAG, "Tarea de Listener terminada.");
            listener->m_listener_task_handle = NULL;
            vTaskDelete(NULL); // Borrarse a sí misma
        }

    } // namespace Transport
} // namespace FlightProxy