#include "FlightProxy/Transport/ListenerTCP.h"
#include "FlightProxy/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include "lwip/sockets.h"
#include <memory>

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "ListenerTCP";

        ListenerTCP::ListenerTCP() : m_mutex(xSemaphoreCreateRecursiveMutex())
        {
        }

        ListenerTCP::~ListenerTCP()
        {
            stopListening();

            // anadimos join de la tarea
            int loops = 0;
            while (true)
            {
                bool task_running;
                {
                    Core::Utils::MutexGuard lock(m_mutex);
                    task_running = (m_listener_task_handle != NULL);
                }

                if (!task_running)
                {
                    break; // La tarea ha terminado
                }

                if (++loops > 100)
                { // Timeout de 2 segundos
                    FP_LOG_E(TAG, "Timeout esperando a listener_task! Fugando mutex.");
                    return; // No borrar el mutex, la tarea sigue viva
                }
                vTaskDelay(pdMS_TO_TICKS(20));
            }

            FP_LOG_I(TAG, "Tarea de Listener terminada. Borrando mutex.");
            vSemaphoreDelete(m_mutex);
        }

        bool ListenerTCP::startListening(uint16_t port)
        {
            Core::Utils::MutexGuard lock(m_mutex);

            if (m_listener_task_handle != NULL)
            {
                FP_LOG_W(TAG, "El listener ya estaba iniciado.");
                return true;
            }

            // 1. Crear el socket
            m_server_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (m_server_sock < 0)
            {
                return false;
            }

            // 2. Configurar dirección y reusar (SO_REUSEADDR)
            struct sockaddr_in dest_addr;
            dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(port);
            int opt = 1;
            setsockopt(m_server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            // 3. Bind
            if (::bind(m_server_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
            {
                FP_LOG_E(TAG, "Error en bind: %d", errno);
                ::close(m_server_sock);
                m_server_sock = -1;
                return false;
            }

            // 4. Listen
            if (::listen(m_server_sock, 2) != 0)
            {
                FP_LOG_E(TAG, "Error en listen: %d", errno);
                ::close(m_server_sock);
                m_server_sock = -1;
                return false;
            }

            // 5. Crear la tarea
            BaseType_t result = xTaskCreate(
                listenerTaskAdapter,
                "tcp_listener_task",
                4096,
                this,
                5,
                &m_listener_task_handle);

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
            Core::Utils::MutexGuard lock(m_mutex);

            if (m_listener_task_handle == NULL)
            {
                return; // Ya está parado
            }

            FP_LOG_I(TAG, "Parando listener...");

            // Cerrar el socket del servidor. Esto hará que
            // accept() en la otra tarea falle inmediatamente.
            if (m_server_sock != -1)
            {
                ::close(m_server_sock);
                m_server_sock = -1;
            }
            // La tarea se limpiará a sí misma. El destructor esperará.
        }

        // --- Tarea de FreeRTOS ---
        void ListenerTCP::listenerTaskAdapter(void *arg)
        {
            ListenerTCP *listener = static_cast<ListenerTCP *>(arg);

            // La tarea se ejecuta...
            listener->listenerTask();

            // ... y cuando termina, se auto-elimina.
            vTaskDelete(NULL);
        }

        void ListenerTCP::listenerTask()
        {
            FP_LOG_I(TAG, "Tarea de Listener iniciada.");

            // Hacemos una copia local del socket (bajo mutex)
            // para no tener que bloquear el mutex en cada 'accept'
            int server_sock_local;
            {
                Core::Utils::MutexGuard lock(m_mutex);
                server_sock_local = m_server_sock;
            }

            while (true)
            {
                struct sockaddr_in source_addr;
                socklen_t addr_len = sizeof(source_addr);

                int client_sock = ::accept(server_sock_local, (struct sockaddr *)&source_addr, &addr_len);

                if (client_sock < 0)
                {
                    // Error. Comprobamos si fue un cierre intencional
                    bool was_stopped;
                    {
                        Core::Utils::MutexGuard lock(m_mutex);
                        was_stopped = (m_server_sock == -1);
                    }

                    if (was_stopped)
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

                // 1. Creamos el objeto y su shared_ptr (el propietario)
                auto new_transport = std::make_shared<SimpleTCP>(client_sock);

                // 2. Creamos un weak_ptr (el observador) a partir del shared_ptr
                auto new_weak_transport = std::weak_ptr(new_transport);

                // 2. Pasamos el weakptr<ITransport>
                if (onNewTransport)
                {
                    onNewTransport(new_weak_transport);
                }
                else
                {
                    FP_LOG_W(TAG, "Cliente aceptado, pero no hay suscriptor en onNewTransport!");
                    // Si no hay suscriptor, debemos cerrar el socket
                    new_transport->close();
                }
            }

            // --- Limpieza de la tarea ---
            FP_LOG_I(TAG, "Tarea de Listener terminada.");
            {
                Core::Utils::MutexGuard lock(m_mutex);
                m_listener_task_handle = NULL; // <-- Avisamos al destructor
            }
        }

    } // namespace Transport
} // namespace FlightProxy