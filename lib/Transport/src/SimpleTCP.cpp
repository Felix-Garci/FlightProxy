#include "FlightProxy/Transport/SimpleTCP.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include "lwip/sockets.h"
#include <vector>

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "SimpleTCP"; // Tag for logging simple TCP

        SimpleTCP::SimpleTCP(int accepted_socket)
            // TU CÓDIGO ORIGINAL ESTABA BIEN: Inicializa m_sock, el task handle y crea el mutex
            : m_sock(accepted_socket), eventTaskHandle_(nullptr), mutex_(xSemaphoreCreateMutex())
        {
            if (mutex_ == NULL)
            {
                FP_LOG_E(TAG, "Error al crear mutex!");
            }
            FP_LOG_I(TAG, "Canal (socket %d): Creado.", m_sock);
        }

        SimpleTCP::~SimpleTCP()
        {
            // TU CÓDIGO ORIGINAL ESTABA BIEN: Limpia el mutex y el socket
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

            if (eventTaskHandle_ != nullptr || m_sock == -1)
            {
                FP_LOG_W(TAG, "Tarea de eventos ya iniciada o socket nulo.");
                return;
            }

            // 'open()' SÓLO crea la tarea.
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
            // TU CÓDIGO ORIGINAL ESTABA PERFECTO
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
            // TU CÓDIGO ORIGINAL ESTABA PERFECTO
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

        // --- Lógica de la Tarea ---
        // (He movido eventTask() aquí dentro)

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