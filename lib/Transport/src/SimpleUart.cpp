#include "FlightProxy/Transport/SimpleUart.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy
{
    namespace Transport
    {
        static const char *TAG = "SimpleUart"; // Tag for logging simple uart

        SimpleUart::SimpleUart(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate)
            : port_(port), txpin_(txpin), rxpin_(rxpin), baudrate_(baudrate), eventTaskHandle_(nullptr),
              queue_(nullptr), rxBuffer_(nullptr), rxbuffersize_(1024),
              mutex_(xSemaphoreCreateRecursiveMutex())
        {
        }

        SimpleUart::~SimpleUart()
        {
            close();
            if (mutex_ != NULL)
            {
                vSemaphoreDelete(mutex_);
                mutex_ = NULL;
            }
        }

        void SimpleUart::open()
        {
            Core::Utils::MutexGuard lock(mutex_);

            // 1. Configurar la UART
            uart_config_t uart_config = {
                .baud_rate = (int)baudrate_,
                .data_bits = UART_DATA_8_BITS,
                .parity = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .rx_flow_ctrl_thresh = 122,
                .source_clk = UART_SCLK_DEFAULT,
                .flags = {0, 0},
            };
            ESP_ERROR_CHECK(uart_param_config(port_, &uart_config));

            // 2. Configurar los pines
            ESP_ERROR_CHECK(uart_set_pin(port_, txpin_, rxpin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

            // 3. Instalar el driver de la UART
            // Se usa rxbuffersize_ para el buffer de RX interno del driver , 0 para indicar que No queremos tx buffer,
            // y se crea una cola de eventos
            ESP_ERROR_CHECK(uart_driver_install(port_, rxbuffersize_, 0, 20, &queue_, 0));

            // 4. Asignar memoria para el buffer de recepción temporal
            // distinto del buffer del driver. este es propiedad de la clase
            rxBuffer_ = (uint8_t *)malloc(rxbuffersize_);

            if (rxBuffer_ == nullptr)
            {
                FP_LOG_E(TAG, "Failed to allocate memory for RX buffer");
                uart_driver_delete(port_); // Limpiamos driver al ver fallo
                return;
            }

            std::shared_ptr<SimpleUart> self_keep_alive = weak_from_this().lock();
            if (!self_keep_alive)
            {
                // El objeto está siendo destruido o no es válido.
                FP_LOG_E(TAG, "open() falló: el objeto está siendo destruido (no se pudo obtener shared_ptr)");

                return; // Salir de la función
            }
            auto *task_arg = new std::shared_ptr<SimpleUart>(std::move(self_keep_alive));

            // 5. Crear la tarea de eventos
            BaseType_t result = xTaskCreate(
                eventTaskAdapter,  // Función de la tarea (el adaptador estático)
                "uart_event_task", // Nombre de la tarea
                4096,              // Tamaño del stack
                task_arg,          // Parámetro de la tarea (puntero a esta instancia)
                10,                // Prioridad
                &eventTaskHandle_  // Handle de la tarea
            );

            if (result != pdPASS)
            {
                FP_LOG_E(TAG, "Fallo al crear la tere UART");
                delete task_arg;
                free(rxBuffer_);
                rxBuffer_ = nullptr;
                uart_driver_delete(port_);
                eventTaskHandle_ = nullptr;
            }
        }

        void SimpleUart::close()
        {
            Core::Utils::MutexGuard lock(mutex_);
            // no se si esta cerrada ya si esto daria erro, esto hay que manejarlo

            // Notificamos a la tarea que queremos cerrar todo.
            uart_driver_delete(port_);
        }

        void SimpleUart::send(const uint8_t *data, size_t len)
        {
            Core::Utils::MutexGuard lock(mutex_);

            // Comprovamos si la tarea buffer siguen vivos
            if (rxBuffer_ == nullptr || eventTaskHandle_ == nullptr)
            {
                FP_LOG_W(TAG, "Intento de envío en UART cerrada.");
                return;
            }

            if (data == nullptr || len == 0)
                return;

            // Escribir datos en la UART
            uart_write_bytes(port_, (const char *)data, len);
        }

        void SimpleUart::eventTaskAdapter(void *arg)
        {
            // 1. Recibimos el puntero al shared_ptr que está en el heap
            auto *self_ptr_on_heap = static_cast<std::shared_ptr<SimpleUart> *>(arg);

            // 2. Obtenemos el puntero 'this'
            SimpleUart *instance = self_ptr_on_heap->get();

            // 3. Llamamos a la tarea, pasando el puntero del heap
            //    La tarea 'eventTask' tomará posesión y lo borrará.
            instance->eventTask(self_ptr_on_heap);
        }

        void SimpleUart::eventTask(std::shared_ptr<SimpleUart> *self_ptr_on_heap)
        {
            // 1. Tomamos posesión del shared_ptr (moviéndolo a nuestro stack)
            std::shared_ptr<SimpleUart> self = std::move(*self_ptr_on_heap);

            // 2. Borramos el puntero del heap (que ahora está vacío)
            delete self_ptr_on_heap;

            // Ahora 'self' (en el stack de esta tarea) mantendrá el objeto
            // SimpleUart vivo mientras la tarea se ejecute.

            if (onOpen)
            {
                onOpen();
            }
            uart_event_t event;
            for (;;) // Bucle infinito de la tarea
            {
                // Esperar por un evento en la cola
                // En el caso de que se llame close() -> uart_delete() este if debuelve false y va al else.
                // Por lo que si este xQueRecive da true estamos seguros de que no se ha eliminado el driver
                if (xQueueReceive(queue_, (void *)&event, portMAX_DELAY))
                {
                    Core::Utils::MutexGuard lock(mutex_);

                    switch (event.type)
                    {
                    // --- Evento: Datos recibidos ---
                    case UART_DATA:
                    {
                        size_t buffered_len;
                        uart_get_buffered_data_len(port_, &buffered_len);

                        // Limitar la lectura al tamaño de nuestro buffer
                        size_t read_len = (buffered_len > rxbuffersize_) ? rxbuffersize_ : buffered_len;

                        // Leer los datos del buffer interno de la UART a nuestro rxBuffer_
                        int rxBytes = uart_read_bytes(port_, rxBuffer_, read_len, pdMS_TO_TICKS(20));

                        if (rxBytes > 0)
                        {
                            // Llamar al callback onData si está definido
                            if (onData)
                            {
                                onData(rxBuffer_, rxBytes);
                            }
                        }
                    }
                    break;

                    // --- Eventos de Error ---
                    case UART_FIFO_OVF:
                        FP_LOG_W(TAG, "HW FIFO Overflow");
                        uart_flush_input(port_);
                        xQueueReset(queue_);
                        break;

                    case UART_BUFFER_FULL:
                        FP_LOG_W(TAG, "Ring Buffer Full");
                        uart_flush_input(port_);
                        xQueueReset(queue_);
                        break;

                    case UART_BREAK:
                        FP_LOG_W(TAG, "UART Break");
                        break;

                    case UART_PARITY_ERR:
                        FP_LOG_E(TAG, "UART Parity Error");
                        break;

                    case UART_FRAME_ERR:
                        FP_LOG_E(TAG, "UART Frame Error");
                        break;

                    // Otros eventos (PATTERN_DET, etc.)
                    default:
                        FP_LOG_I(TAG, "UART event type: %d", event.type);
                        break;
                    }
                }
                else
                {
                    // xQueueReceive falló (probablemente porque se llamó a close()
                    // y uart_driver_delete() liberó la cola).
                    // Salir del bucle para que la tarea termine.
                    FP_LOG_I(TAG, "UART event task stopping.");
                    break;
                }
            } // FIN del for(;;)

            FP_LOG_I(TAG, "Tarea UART terminando. Realizando limpieza...");

            {
                Core::Utils::MutexGuard lock(mutex_);
                // Hacemos la limpieza
                if (rxBuffer_ != nullptr)
                {
                    free(rxBuffer_);
                    rxBuffer_ = nullptr;
                }
                eventTaskHandle_ = nullptr;
                // 'queue_' ya está liberada por uart_driver_delete()
            }

            // 8. Notificar a la App (FUERA del mutex)
            if (onClose)
            {
                onClose();
            }

            vTaskDelete(NULL);
        }
    }
}