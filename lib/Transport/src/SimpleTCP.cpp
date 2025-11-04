#include "FlightProxy/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h" // Para getaddrinfo
#include <vector>
#include <cstring> // Para strncpy

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "SimpleTCP";

        SimpleTCP::SimpleTCP(int accepted_socket)
            : m_sock(accepted_socket), port_(0), eventTaskHandle_(nullptr),
              mutex_(xSemaphoreCreateRecursiveMutex())
        {
            ip_[0] = '\0';
        }

        SimpleTCP::SimpleTCP(const char *ip, uint16_t port)
            : port_(port), m_sock(-1), eventTaskHandle_(nullptr),
              mutex_(xSemaphoreCreateRecursiveMutex())
        {
            if (ip != nullptr)
            {
                // Copia como máximo 15 caracteres para dejar espacio para el '\0'
                strncpy(ip_, ip, sizeof(ip_) - 1);
                // Asegura que la cadena siempre termine en nulo, incluso si la IP es muy larga
                ip_[sizeof(ip_) - 1] = '\0';
            }
            else
            {
                // Si la IP es nula, deja el buffer vacío
                ip_[0] = '\0';
                FP_LOG_W(TAG, "Constructor de cliente llamado con IP nula.");
            }
        }

        SimpleTCP::~SimpleTCP()
        {
            if (m_sock != -1)
            {
                FP_LOG_W(TAG, "Llamando al destructor sin tener cerrado el canal.");
                close();
            }

            // 2. Espera a que la eventTask termine
            //    La tarea pondrá 'eventTaskHandle_' a NULL (dentro de un mutex)
            //    cuando haya terminado su limpieza.
            FP_LOG_I(TAG, "Destructor esperando a que la tarea finalice...");
            int loop_count = 0;
            bool task_alive = true;
            while (task_alive)
            {
                // Comprobamos la variable *dentro* del mutex
                {
                    Core::Utils::MutexGuard lock(mutex_);
                    if (eventTaskHandle_ == nullptr)
                    {
                        task_alive = false;
                    }
                } // Mutex se libera

                if (task_alive)
                {
                    vTaskDelay(pdMS_TO_TICKS(20));
                    if (++loop_count > 100) // Timeout de 2 segundos
                    {
                        FP_LOG_E(TAG, "Timeout esperando a eventTask! Fugando mutex.");
                        // La tarea sigue viva y podría usar el mutex
                        // NO PODEMOS borrar el mutex, o crasheará.
                        return;
                    }
                }
            }

            // 3. Ahora que la tarea está muerta, podemos borrar el mutex
            FP_LOG_I(TAG, "eventTask terminada. Limpiando mutex.");
            if (mutex_)
            {
                vSemaphoreDelete(mutex_);
                mutex_ = nullptr;
            }
            FP_LOG_I(TAG, "Canal destruido.");
        }

        void SimpleTCP::open()
        {
            Core::Utils::MutexGuard lock(mutex_);

            if (eventTaskHandle_ != nullptr)
            {
                FP_LOG_W(TAG, "Tarea de eventos ya iniciada.");
                return;
            }

            // --- Lógica de conexión para el MODO CLIENTE ---
            if (m_sock == -1)
            {
                // Si no tenemos socket, es un cliente que debe conectarse.
                if (ip_[0] == '\0' || port_ == 0)
                {
                    FP_LOG_E(TAG, "No se puede abrir: IP o puerto no configurados para el cliente.");
                    return;
                }

                FP_LOG_I(TAG, "Modo cliente: Intentando conectar a %s:%u...", ip_, port_);

                // 1. Resolver la dirección (getaddrinfo es más robusto)
                struct addrinfo hints = {};
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                struct addrinfo *res;
                char port_str[6];
                sprintf(port_str, "%u", port_);

                int err = getaddrinfo(ip_, port_str, &hints, &res);
                if (err != 0 || res == nullptr)
                {
                    FP_LOG_E(TAG, "Error en getaddrinfo para '%s': %d", ip_, err);
                    return; // No se puede conectar
                }

                // 2. Crear el socket
                int sock = ::socket(res->ai_family, res->ai_socktype, 0);
                if (sock < 0)
                {
                    FP_LOG_E(TAG, "Error al crear socket cliente: %d", errno);
                    freeaddrinfo(res);
                    return;
                }

                // 3. Conectar
                if (::connect(sock, res->ai_addr, res->ai_addrlen) != 0)
                {
                    FP_LOG_E(TAG, "Error en connect a %s:%u: %d", ip_, port_, errno);
                    ::close(sock); // Limpiar el socket fallido
                    freeaddrinfo(res);
                    return;
                }

                freeaddrinfo(res); // Liberar memoria de getaddrinfo
                FP_LOG_I(TAG, "Conectado con éxito! Nuevo socket: %d", sock);
                m_sock = sock; // Guardar el socket recién conectado
            }

            // --- Lógica común: Iniciar la tarea de lectura ---
            BaseType_t result = xTaskCreate(
                eventTaskAdapter, // La función adaptadora estática
                "tcp_event_task", // Nombre de la tarea
                4096,             // Stack
                this,             // Puntero 'this' como argumento
                5,                // Prioridad
                &eventTaskHandle_ // Handle de la tarea
            );

            if (result != pdPASS)
            {
                eventTaskHandle_ = nullptr;
                FP_LOG_E(TAG, "Error al crear la tarea de eventos TCP");

                if (m_sock != -1)
                {
                    ::close(m_sock);
                    m_sock = -1;
                }
            }
        }

        void SimpleTCP::close()
        {
            Core::Utils::MutexGuard lock(mutex_);
            if (m_sock == -1)
            {
                return;
            }

            // Esta es la forma correcta de cierre asíncrono:
            // 1. Marcamos el socket para cierre.
            // 2. La eventTask que está bloqueada en ::recv() se despertará con error.
            // 3. La eventTask se encargará de la limpieza (::close, onClose, etc).
            FP_LOG_I(TAG, "Canal (socket %d): Solicitando cierre (shutdown)...", m_sock);
            ::shutdown(m_sock, SHUT_RDWR);
        }

        void SimpleTCP::send(const uint8_t *data, size_t len)
        {
            Core::Utils::MutexGuard lock(mutex_);

            if (m_sock == -1)
            {
                FP_LOG_W(TAG, "Canal: Intento de envío en socket cerrado.");
                return;
            }
            if (data == nullptr || len == 0)
            {
                FP_LOG_W(TAG, "Canal: Intento de envío datos vacios.");
                return;
            }
            // Tenemos que asegurarnos que todo el contenido de data se envia
            size_t total_sent = 0;
            while (total_sent < len)
            {
                int sent_now = ::send(m_sock, data + total_sent, len - total_sent, 0);
                if (sent_now < 0)
                {
                    FP_LOG_E(TAG, "Canal (socket %d): Error en send(): %d. Abortando envío.", m_sock, errno);

                    // Si el envío falla, es un error grave.
                    // Cerramos la conexión para notificar a la tarea.
                    ::shutdown(m_sock, SHUT_RDWR);
                    return;
                }
                total_sent += sent_now;
            }
        }

        void SimpleTCP::eventTaskAdapter(void *arg)
        {
            SimpleTCP *instance = static_cast<SimpleTCP *>(arg);
            instance->eventTask();
        }

        void SimpleTCP::eventTask()
        {
            if (onOpen)
            {
                onOpen();
            }

            std::vector<uint8_t> rx_buffer(1024);

            FP_LOG_I(TAG, "Tarea Iniciada.");

            // 5. Bucle de lectura
            while (true)
            {
                int len = ::recv(m_sock, rx_buffer.data(), rx_buffer.size(), 0);

                if (len > 0)
                {
                    if (onData)
                    {
                        onData(rx_buffer.data(), len);
                    }
                }
                else if (len == 0)
                {
                    FP_LOG_I(TAG, "Cliente cerró la conexión.");
                    break;
                }
                else
                {
                    // --- Error en la conexión (len < 0) ---
                    // Esto también ocurre si 'close()' llamó a 'shutdown()'.
                    FP_LOG_E(TAG, "Error en recv(): %d. Error en la conexion o que se llamo a this->close()", errno);
                    break;
                }
            }

            // --- Limpieza segura de la Tarea ---

            int sock_to_close = -1;

            // 1. Modificamos el estado del objeto DENTRO del mutex
            {
                Core::Utils::MutexGuard lock(mutex_);

                // Comprobamos si el socket ya fue cerrado (p.ej. por open() fallido)
                if (m_sock != -1)
                {
                    sock_to_close = m_sock;
                    m_sock = -1;
                }
                // Poner esto a NULL le dice al destructor que hemos terminado.
                eventTaskHandle_ = nullptr;
            } // El mutex se libera aquí

            // 2. Limpiamos los recursos (socket) FUERA del mutex
            if (sock_to_close != -1)
            {
                ::close(sock_to_close);
            }

            // 3. Llamamos al callback (FUERA del mutex)
            //    Esto es más seguro, ya que evita deadlocks si 'onClose'
            //    intenta llamar de nuevo a 'open()' o 'close()'.
            if (onClose)
            {
                onClose();
            }

            FP_LOG_I(TAG, "Tarea terminada.");
            vTaskDelete(NULL); // La tarea se autodestruye
        }

    } // namespace Transport
} // namespace FlightProxy