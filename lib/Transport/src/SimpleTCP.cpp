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
            : m_sock(accepted_socket), port_(0), eventTaskHandle_(nullptr), mutex_(xSemaphoreCreateMutex())
        {
            if (mutex_ == NULL)
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
            if (mutex_)
            {
                vSemaphoreDelete(mutex_);
                mutex_ = nullptr;
            }
            if (m_sock != -1)
            {
                FP_LOG_W(TAG, "Canal (socket %d): Destruido sin cerrar limpiamente!", m_sock);
                ::close(m_sock); // Cierre de emergencia (fuga de file descriptor)
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
            ::shutdown(m_sock, SHUT_RDWR);
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

            int sent = ::send(m_sock, data, len, 0);
            if (sent < 0)
            {
                FP_LOG_E(TAG, "Canal (socket %d): Error en send(): %d", m_sock, errno);
            }
        }

        void SimpleTCP::eventTaskAdapter(void *arg)
        {
            // 1. Recuperar el puntero 'this'
            SimpleTCP *transport = static_cast<SimpleTCP *>(arg);
            if (transport == nullptr)
            {
                FP_LOG_E(TAG, "eventTaskAdapter recibió un argumento nulo!");
                vTaskDelete(NULL);
                return;
            }

            // 2. ¡EL "SEGURO DE VIDA"!
            // Obtenemos un shared_ptr a nosotros mismos.
            // El objeto no será destruido mientras 'self' exista (en el stack de esta tarea).
            std::shared_ptr<SimpleTCP> self = transport->shared_from_this();

            // 3. Tarea iniciada. Ahora SÍ llamamos a onOpen
            if (self->onOpen)
            {
                self->onOpen();
            }

            // 4. Preparar bucle de lectura
            std::vector<uint8_t> rx_buffer(1024);
            // Copia local del socket (buena práctica)
            int sock = self->m_sock;
            FP_LOG_I(TAG, "Tarea (socket %d): Iniciada.", sock);

            // 5. Bucle de lectura
            while (true)
            {
                int len = ::recv(sock, rx_buffer.data(), rx_buffer.size(), 0);

                if (len > 0)
                {
                    // --- Datos recibidos ---
                    // Usamos 'self' para llamar a los callbacks
                    if (self->onData)
                    {
                        self->onData(rx_buffer.data(), len);
                    }
                }
                else if (len == 0)
                {
                    // --- Conexión cerrada por el cliente ---
                    FP_LOG_I(TAG, "Tarea (socket %d): Cliente cerró la conexión.", sock);
                    break; // Salir del bucle
                }
                else
                {
                    // --- Error en la conexión (len < 0) ---
                    // Esto también ocurre si 'close()' llamó a 'shutdown()'.
                    FP_LOG_E(TAG, "Tarea (socket %d): Error en recv(): %d. Conexión cerrada.", sock, errno);
                    break; // Salir del bucle
                }
            }

            // 6. --- Limpieza (fuera del bucle) ---

            // Notificar que la conexión se cerró
            if (self->onClose)
            {
                self->onClose();
            }

            // 7. Cerrar el socket (protegido por mutex)
            // Es CRÍTICO usar 'self->' para acceder a miembros
            // en este punto, no 'transport->'.
            {
                // Usamos 'self->mutex_'
                FlightProxy::Core::Utils::MutexGuard MutexGuard(self->mutex_);
                if (self->m_sock != -1)
                {
                    FP_LOG_I(TAG, "Tarea (socket %d): Cerrando file descriptor.", sock);
                    ::close(self->m_sock); // Cierre real
                    self->m_sock = -1;     // Marcar como inválido
                }
            }

            // 8. Tarea completada. Borrarse a sí misma.
            FP_LOG_I(TAG, "Tarea (socket %d): Terminando.", sock);

            // Limpiamos el handle en la clase
            self->eventTaskHandle_ = nullptr;

            // Borramos la tarea de FreeRTOS
            vTaskDelete(NULL);

            // Al salir de esta función, el 'std::shared_ptr self' se destruye.
            // Si el Manager ya no tenía una copia, el objeto será
            // destruido (se llamará a ~SimpleTCP) ahora mismo, de forma segura.
        }

    } // namespace Transport
} // namespace FlightProxy