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
              mutex_(xSemaphoreCreateMutex()), taskworking_(xSemaphoreCreateBinary())
        {
            if (mutex_ == NULL || taskworking_ == NULL)
            {
                FP_LOG_E(TAG, "Error al crear mutex!");
            }

            ip_[0] = '\0';
            FP_LOG_I(TAG, "Canal (socket %d): Creado.", m_sock);
        }
        SimpleTCP::SimpleTCP(const char *ip, uint16_t port)
            : port_(port), m_sock(-1), eventTaskHandle_(nullptr), mutex_(xSemaphoreCreateMutex())
        {
            if (mutex_ == NULL)
            {
                FP_LOG_E(TAG, "Error al crear mutex!");
            }

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

            if (mutex_)
            {
                vSemaphoreDelete(mutex_);
                mutex_ = nullptr;
            }
            if (taskworking_)
            {
                vSemaphoreDelete(taskworking_);
                taskworking_ = nullptr;
            }

            FP_LOG_I(TAG, "Canal (socket %d): Destruido.", m_sock);
        }

        void SimpleTCP::open()
        {
            FlightProxy::Core::Utils::MutexGuard MutexGuard(mutex_);

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
            // La tarea misma es la que debe llamar a 'onOpen()'
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
            }
        }

        void SimpleTCP::close()
        {
            FlightProxy::Core::Utils::MutexGuard lock(mutex_);
            if (m_sock == -1)
            {
                return;
            }
            FP_LOG_I(TAG, "Canal (socket %d): Solicitando cierre...", m_sock);
            ::shutdown(m_sock, SHUT_RDWR); // Le dice al ::recv que ya esta. pero no elimina todavia el soket

            // Close Bloqueante para esperar a que de verdad se ha cerrado
            FlightProxy::Core::Utils::MutexGuard MutexGuard(taskworking_);
        }

        void SimpleTCP::send(const uint8_t *data, size_t len)
        {
            FlightProxy::Core::Utils::MutexGuard MutexGuard(mutex_);

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
                    return;
                }
                total_sent += sent_now;
            }
        }

        void SimpleTCP::eventTaskAdapter(void *arg)
        {
            SimpleTCP *transport = static_cast<SimpleTCP *>(arg);

            if (transport->onOpen)
            {
                transport->onOpen();
            }

            // Ocupamos el semaforo para que desde el close sepamos cuando esto esta terminado ya
            FlightProxy::Core::Utils::MutexGuard MutexGuard(transport->taskworking_);

            std::vector<uint8_t> rx_buffer(1024);

            FP_LOG_I(TAG, "Tarea Iniciada.");

            // 5. Bucle de lectura
            while (true)
            {
                int len = ::recv(transport->m_sock, rx_buffer.data(), rx_buffer.size(), 0);

                if (len > 0)
                {
                    if (transport->onData)
                    {
                        transport->onData(rx_buffer.data(), len);
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
            ::close(transport->m_sock);
            transport->m_sock = -1;
            transport->eventTaskHandle_ = nullptr;
            if (transport->onClose)
            {
                transport->onClose();
            }
            FP_LOG_I(TAG, "Tarea terminada.");
            // Al no poner vTaskDelete(NULL); freertos se encarga de eliminar los objetos creados en este contexto y de eliminar la clase
        }

    } // namespace Transport
} // namespace FlightProxy