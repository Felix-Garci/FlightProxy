#include "FlightProxy/Transport/SimpleTCP.h"

#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "SimpleTCP"; // Tag for logging simple TCP

        SimpleTCP::SimpleTCP(struct netconn *clientSocket)
            : clientSocket_(clientSocket), eventTaskHandle_(nullptr), mutex_(xSemaphoreCreateMutex())
        {
        }

        SimpleTCP::~SimpleTCP()
        {
            close();
            if (mutex_)
            {
                vSemaphoreDelete(mutex_);
                mutex_ = nullptr;
            }
        }

        void SimpleTCP::open()
        {
            // Implementation of TCP opening logic
            FlightProxy::Core::Utils::MutexGuard MutexGuard(mutex_);

            if (eventTaskHandle_ != nullptr || clientSocket_ == nullptr)
            {
                FP_LOG_W(TAG, "TCP event task already running or client socket is null");
                return;
            }

            BaseType_t result = xTaskCreate(
                eventTaskAdapter, // La función adaptadora estática
                "tcp_event_task", // Nombre de la tarea (para debugging)
                4096,             // Tamaño de la pila (ajustar según sea necesario)
                this,             // Parámetro (puntero a esta instancia)
                5,                // Prioridad de la tarea
                &eventTaskHandle_ // Puntero para almacenar el handle de la tarea
            );

            if (result != pdPASS)
            {
                eventTaskHandle_ = nullptr;
                FP_LOG_E(TAG, "Failed to create TCP event task");
            }
            else
            {
                // Llamar al callback onOpen si está definido
                if (onOpen)
                {
                    onOpen();
                }
            }
        }

        void SimpleTCP::close()
        {
            // Implementation of TCP closing logic
            struct netconn *socketToClose = nullptr;

            { // Proteger el acceso a clientSocket_ con el mutex
                FlightProxy::Core::Utils::MutexGuard lock(mutex_);

                if (clientSocket_ == nullptr)
                {
                    return;
                }
                socketToClose = clientSocket_;
                clientSocket_ = NULL;
            }

            // Ahora, fuera del mutex, cerramos y borramos el socket.
            // Esto hará que netconn_recv() en eventTask falle.
            if (socketToClose != NULL)
            {
                netconn_close(socketToClose);
                netconn_delete(socketToClose);
            }

            // NO LLAMAMOS a vTaskDelete(taskHandle_).
            // La propia eventTask se encargará de limpiarse.
        }

        void SimpleTCP::send(const uint8_t *data, size_t len)
        {
            // Implementation of TCP sending logic
            FlightProxy::Core::Utils::MutexGuard MutexGuard(mutex_);

            if (clientSocket_ == nullptr || data == nullptr || len == 0)
            {
                FP_LOG_E(TAG, "Invalid socket or data to send");
                return;
            }
            err_t err = netconn_write(clientSocket_, data, len, NETCONN_COPY);
            if (err != ERR_OK)
            {
                FP_LOG_E(TAG, "Failed to send data over TCP: %d", err);
            }
        }

        void SimpleTCP::eventTask()
        {
            struct netbuf *buf;
            err_t err;

            // Bucle principal: esperar datos
            // netconn_recv SE BLOQUEA INDEFINIDAMENTE
            while ((err = netconn_recv(clientSocket_, &buf)) == ERR_OK)
            {
                // --- Datos recibidos ---
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

                netbuf_delete(buf);
            }

            // --- Tarea terminando ---
            // Si salimos del bucle, significa que err != ERR_OK.
            // Esto pasa si:
            // 1. El cliente cerró la conexión (ERR_CLSD)
            // 2. Nuestra propia función close() llamó a netconn_close() (ERR_ABRT)

            // La función adaptadora (eventTaskAdapter) se encargará
            // de poner taskHandle_ a NULL y de llamar a vTaskDelete.

            if (onClose)
            {
                onClose();
            }
        }

        void SimpleTCP::eventTaskAdapter(void *arg)
        {
            SimpleTCP *instance = static_cast<SimpleTCP *>(arg);

            if (instance)
            {
                // Ejecutar la tarea
                instance->eventTask();

                // --- Limpieza después de que eventTask termine ---

                // Limpiar el handle de la tarea para que open() pueda
                // ser llamado de nuevo si se desea.
                {
                    FlightProxy::Core::Utils::MutexGuard MutexGuard(instance->mutex_);
                    instance->eventTaskHandle_ = NULL;
                }
            }

            // Tarea completada, eliminarse a sí misma.
            vTaskDelete(NULL);
        }
    }
}