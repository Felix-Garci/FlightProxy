#include "FlightProxy/Transport/SimpleUDP.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "SimpleUDP"; // Tag for logging simple UDP

        SimpleUDP::SimpleUDP(uint16_t localPort)
            : udpSocket_(nullptr), eventTaskHandle_(nullptr), localPort_(localPort),
              remotePort_(0), remoteMutex_(xSemaphoreCreateMutex())
        {
            ip_addr_set_zero(&remoteIP_);
        }

        SimpleUDP::~SimpleUDP()
        {
            close();
            if (remoteMutex_)
            {
                vSemaphoreDelete(remoteMutex_);
                remoteMutex_ = nullptr;
            }
        }

        void SimpleUDP::open()
        {
            // Evitar doble apertura
            if (udpSocket_ != nullptr || remoteMutex_ == nullptr)
            {
                return;
            }

            // 1. Crear un nuevo socket UDP
            udpSocket_ = netconn_new(NETCONN_UDP);
            if (udpSocket_ == nullptr)
            {
                FP_LOG_E(TAG, "Failed to create UDP socket");
                return;
            }

            // 2. Enlazar (bind) el socket al puerto local y a cualquier IP
            err_t err = netconn_bind(udpSocket_, IP_ADDR_ANY, localPort_);
            if (err != ERR_OK)
            {
                FP_LOG_E(TAG, "Failed to bind UDP socket to port %u: %d", localPort_, err);
                netconn_delete(udpSocket_);
                udpSocket_ = nullptr;
                return;
            }

            // 3. Crear la tarea de FreeRTOS para recibir datos
            BaseType_t result = xTaskCreate(
                eventTaskAdapter, // Función adaptadora estática
                "udp_event_task", // Nombre de la tarea
                4096,             // Tamaño de la pila
                this,             // Puntero 'this' como argumento
                5,                // Prioridad
                &eventTaskHandle_ // Handle de la tarea
            );

            if (result != pdPASS)
            {
                // Error: no se pudo crear la tarea
                netconn_delete(udpSocket_);
                udpSocket_ = nullptr;
                eventTaskHandle_ = nullptr;
            }
        }

        void SimpleUDP::close()
        {
            netconn *socketToKill = nullptr;
            {
                Core::Utils::MutexGuard MutexGuard(remoteMutex_);

                socketToKill = udpSocket_;

                // Poner a nulo para evitar uso concurrente
                udpSocket_ = nullptr;
                eventTaskHandle_ = nullptr;
            }
            // 2. Eliminar el socket lwIP.
            // ESTO ES IMPORTANTE: netconn_delete() desbloqueará
            // a la tarea que esté en netconn_recv(), haciendo que falle
            // y la tarea termine limpiamente.
            if (socketToKill != nullptr)
            {
                netconn_delete(socketToKill);
            }
        }

        void SimpleUDP::send(const uint8_t *data, size_t size)
        {
            if (data == nullptr || size == 0 || remoteMutex_ == nullptr)
            {
                return;
            }

            ip_addr_t destIP;
            uint16_t destPort;
            netconn *sock;

            { // --- Sección Crítica ---
                Core::Utils::MutexGuard MutexGuard(remoteMutex_);
                // Copiar las variables compartidas a locales
                destIP = remoteIP_;
                destPort = remotePort_;
                sock = udpSocket_;
            } // --- Fin Sección Crítica ---

            // Validar que tenemos un destino y un socket
            if (sock == nullptr || destPort == 0 || ip_addr_isany(&destIP))
            {
                // No tenemos un destino (nadie nos ha enviado nada)
                // o el socket está cerrado.
                return;
            }

            // 1. Crear un netbuf (el contenedor de lwIP para paquetes)
            struct netbuf *buf = netbuf_new();
            if (buf == nullptr)
            {
                FP_LOG_E(TAG, "Failed to create netbuf for UDP send");
                return; // Error de memoria
            }

            // 2. Asignar memoria dentro del netbuf
            void *ptr = netbuf_alloc(buf, size);
            if (ptr == nullptr)
            {
                netbuf_delete(buf); // Limpiar
                return;             // Error de memoria
            }

            // 3. Copiar nuestros datos al netbuf
            memcpy(ptr, data, size);

            // 4. Enviar el netbuf al destino
            err_t err = netconn_sendto(sock, buf, &destIP, destPort);

            // 5. Liberar el netbuf
            netbuf_delete(buf);

            if (err != ERR_OK)
            {
                FP_LOG_E(TAG, "Failed to send UDP data: %d", err);
            }
        }

        void SimpleUDP::eventTask()
        {
            struct netbuf *buf;
            err_t err;

            // Obtener el socket de forma segura (aunque no debería cambiar
            // si no llamamos a close())
            netconn *sock;

            {
                Core::Utils::MutexGuard MutexGuard(remoteMutex_);
                sock = udpSocket_;
            }

            if (sock == nullptr)
            {
                return; // El socket no se creó bien, salir de la tarea
            }

            // Bucle principal: esperar datos
            // netconn_recv se bloqueará indefinidamente hasta que llegue un paquete
            // o hasta que netconn_delete(sock) sea llamado desde close().
            while ((err = netconn_recv(sock, &buf)) == ERR_OK)
            {
                // --- Datos recibidos ---

                // --- Sección Crítica: Actualizar IP/Puerto del remitente ---
                {
                    Core::Utils::MutexGuard MutexGuard(remoteMutex_);
                    // Guardar quién nos envió este paquete
                    remoteIP_ = *netbuf_fromaddr(buf);
                    remotePort_ = netbuf_fromport(buf);
                }
                // --- Fin Sección Crítica ---

                // Procesar los datos del buffer
                do
                {
                    void *data;
                    u16_t len;
                    netbuf_data(buf, &data, &len);

                    if (onData)
                    {
                        onData(static_cast<const uint8_t *>(data), len);
                    }

                } while (netbuf_next(buf) >= 0);

                // Liberar el buffer de lwIP
                netbuf_delete(buf);
            }

            // --- Tarea terminando ---
            // Si salimos del bucle, err != ERR_OK.
            // Esto significa que netconn_delete() fue llamado desde close()
            // o hubo un error fatal. La tarea debe terminar.

            // La función adaptadora (eventTaskAdapter) se encargará
            // de poner taskHandle_ a NULL y de llamar a vTaskDelete.
            if (onClose)
            {
                onClose();
            }
        }

        void SimpleUDP::eventTaskAdapter(void *arg)
        {
            SimpleUDP *instance = static_cast<SimpleUDP *>(arg);
            if (instance != nullptr)
            {
                // Ejecutar la tarea
                instance->eventTask();

                // --- Limpieza después de que eventTask termine ---
                if (instance->remoteMutex_ != nullptr)
                {
                    {
                        Core::Utils::MutexGuard MutexGuard(instance->remoteMutex_);
                        instance->eventTaskHandle_ = nullptr;
                    }
                }
            }

            // Tarea completada, eliminarse a sí misma.
            vTaskDelete(NULL);
        }
    }
}