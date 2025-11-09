#include "FlightProxy/PlatformESP32/Transport/SimpleUDP.h" // Asegúrate de que la ruta sea correcta
#include "FlightProxy/Core/Utils/Logger.h"

#include "lwip/sockets.h"
#include <vector>
#include <cstring> // Para memset

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Transport
        {
            static const char *TAG = "SimpleUDP";

            SimpleUDP::SimpleUDP(uint16_t port)
                : m_port(port),
                  m_last_sender_len(sizeof(m_last_sender_addr)),
                  m_has_last_sender(false),
                  eventTaskHandle_(nullptr),
                  mutex_(xSemaphoreCreateRecursiveMutex())
            {
                memset(&m_last_sender_addr, 0, sizeof(m_last_sender_addr));
                FP_LOG_I(TAG, "Canal UDP creado para el puerto %u", m_port);
            }

            SimpleUDP::~SimpleUDP()
            {
                FP_LOG_I(TAG, "eventTask terminada. Limpiando mutex.");
                if (mutex_)
                {
                    vSemaphoreDelete(mutex_);
                    mutex_ = nullptr;
                }
                FP_LOG_I(TAG, "Canal destruido.");
            }

            void SimpleUDP::open()
            {
                Core::Utils::MutexGuard lock(mutex_);

                if (eventTaskHandle_ != nullptr)
                {
                    FP_LOG_W(TAG, "Tarea de eventos ya iniciada.");
                    return;
                }

                // --- Lógica de "conexión" (bind) para MODO SERVIDOR UDP ---
                if (m_sock == -1)
                {
                    FP_LOG_I(TAG, "Modo servidor UDP: Intentando escuchar en el puerto %u...", m_port);

                    // 1. Crear el socket UDP
                    m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if (m_sock < 0)
                    {
                        FP_LOG_E(TAG, "Error al crear socket UDP: %d", errno);
                        return;
                    }

                    // 2. Configurar la dirección para bind (escuchar en CUALQUIER IP)
                    struct sockaddr_in bind_addr;
                    memset(&bind_addr, 0, sizeof(bind_addr));
                    bind_addr.sin_family = AF_INET;
                    bind_addr.sin_port = htons(m_port);
                    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

                    // 3. Bind
                    if (::bind(m_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0)
                    {
                        FP_LOG_E(TAG, "Error en bind al puerto %u: %d", m_port, errno);
                        ::close(m_sock); // Limpiar el socket fallido
                        m_sock = -1;
                        return;
                    }
                    FP_LOG_I(TAG, "Escuchando con éxito en UDP:%u! Socket: %d", m_port, m_sock);
                }

                // --- Lógica común: Iniciar la tarea de lectura (COPIADA DE TCP) ---
                std::shared_ptr<SimpleUDP> self_keep_alive = weak_from_this().lock();
                if (!self_keep_alive)
                {
                    FP_LOG_E(TAG, "open() falló: el objeto está siendo destruido (no se pudo obtener shared_ptr)");
                    if (m_sock != -1)
                    {
                        ::close(m_sock);
                        m_sock = -1;
                    }
                    return;
                }

                auto *task_arg = new std::shared_ptr<SimpleUDP>(std::move(self_keep_alive));

                BaseType_t result = xTaskCreate(
                    eventTaskAdapter,
                    "udp_event_task", // Nombre de la tarea
                    4096,             // Stack
                    task_arg,         // Puntero shared que esta en el heap
                    5,                // Prioridad
                    &eventTaskHandle_ // Handle de la tarea
                );

                if (result != pdPASS)
                {
                    eventTaskHandle_ = nullptr;
                    FP_LOG_E(TAG, "Error al crear la tarea de eventos UDP");
                    delete task_arg; // Liveramos memoria pq el task no la va a liberar
                    if (m_sock != -1)
                    {
                        ::close(m_sock);
                        m_sock = -1;
                    }
                }
            }

            void SimpleUDP::close()
            {
                Core::Utils::MutexGuard lock(mutex_);
                if (m_sock == -1)
                {
                    return;
                }

                // Usamos shutdown(SHUT_RD) para despertar a recvfrom()
                // Esto es más limpio que cerrar el socket desde este hilo.
                // La eventTask se encargará de llamar a ::close().
                FP_LOG_I(TAG, "Canal (socket %d): Solicitando cierre (shutdown)...", m_sock);
                ::shutdown(m_sock, SHUT_RD); // SHUT_RD es suficiente para recvfrom
            }

            void SimpleUDP::send(const uint8_t *data, size_t len)
            {
                Core::Utils::MutexGuard lock(mutex_);

                if (m_sock == -1)
                {
                    FP_LOG_W(TAG, "Canal: Intento de envío en socket cerrado.");
                    return;
                }
                if (!m_has_last_sender)
                {
                    FP_LOG_W(TAG, "Canal: Intento de envío sin un destinatario (aún no se ha recibido nada).");
                    return;
                }
                if (data == nullptr || len == 0)
                {
                    FP_LOG_W(TAG, "Canal: Intento de envío datos vacios.");
                    return;
                }

                // Enviar al último remitente conocido
                int sent_now = ::sendto(m_sock, data, len, 0,
                                        (struct sockaddr *)&m_last_sender_addr,
                                        m_last_sender_len);

                if (sent_now < 0)
                {
                    FP_LOG_E(TAG, "Canal (socket %d): Error en sendto(): %d.", m_sock, errno);
                    // Para UDP, un error de envío no es (normalmente) fatal para la conexión.
                    // No llamamos a shutdown() aquí, a diferencia de TCP.
                }
            }

            // --- Tareas (Adaptador y Tarea de Eventos) ---

            void SimpleUDP::eventTaskAdapter(void *arg)
            {
                // 1. Recibimos el puntero al shared_ptr que está en el heap
                auto *self_ptr_on_heap = static_cast<std::shared_ptr<SimpleUDP> *>(arg);

                // 2. Obtenemos el puntero 'this'
                SimpleUDP *instance = self_ptr_on_heap->get();

                // 3. Llamamos a la tarea, pasando el puntero del heap
                instance->eventTask(self_ptr_on_heap);
            }

            void SimpleUDP::eventTask(std::shared_ptr<SimpleUDP> *self_ptr_on_heap)
            {
                // 1. Tomamos posesión del shared_ptr
                std::shared_ptr<SimpleUDP> self = std::move(*self_ptr_on_heap);

                // 2. Borramos el puntero del heap
                delete self_ptr_on_heap;

                // 'self' mantendrá el objeto vivo
                if (onOpen)
                {
                    onOpen();
                }

                // Un buffer más grande es apropiado para UDP (cerca de MTU)
                std::vector<uint8_t> rx_buffer(1500);

                FP_LOG_I(TAG, "Tarea Iniciada.");

                // Bucle de lectura
                while (true)
                {
                    // Preparar para recibir la dirección del remitente
                    struct sockaddr_in sender_addr;
                    socklen_t sender_len = sizeof(sender_addr);

                    int len = ::recvfrom(m_sock, rx_buffer.data(), rx_buffer.size(), 0,
                                         (struct sockaddr *)&sender_addr, &sender_len);

                    if (len > 0)
                    {
                        // Guardar el remitente para futuros 'send()'
                        {
                            Core::Utils::MutexGuard lock(mutex_);
                            m_last_sender_addr = sender_addr;
                            m_last_sender_len = sender_len;
                            m_has_last_sender = true;
                        }

                        if (onData)
                        {
                            // Opcional: Loguear de quién recibimos
                            char sender_ip[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
                            FP_LOG_I(TAG, "Recibido %d bytes de %s:%u", len, sender_ip, ntohs(sender_addr.sin_port));
                            FP_LOG_I(TAG, "Contenido: %.*s", len, rx_buffer.data());
                            onData(rx_buffer.data(), len);
                        }
                    }
                    else if (len == 0)
                    {
                        // Esto no debería ocurrir con UDP (sockets DGRAM)
                        FP_LOG_I(TAG, "recvfrom devolvió 0. Raro.");
                    }
                    else
                    {
                        // Error (len < 0)
                        // Ocurre si 'close()' llamó a 'shutdown()'.
                        FP_LOG_E(TAG, "Error en recvfrom(): %d. Cerrando tarea.", errno);
                        break;
                    }
                }

                // --- Limpieza segura de la Tarea ---
                int sock_to_close = -1;

                {
                    Core::Utils::MutexGuard lock(mutex_);
                    if (m_sock != -1)
                    {
                        sock_to_close = m_sock;
                        m_sock = -1;
                    }
                    eventTaskHandle_ = nullptr;
                    m_has_last_sender = false; // Invalidar el remitente
                }

                if (sock_to_close != -1)
                {
                    ::close(sock_to_close);
                }

                if (onClose)
                {
                    onClose();
                }

                FP_LOG_I(TAG, "Tarea terminada.");
                vTaskDelete(NULL); // La tarea se autodestruye
            }
        } // namespace Transport
    }
} // namespace FlightProxy