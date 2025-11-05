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
            Core::Utils::MutexGuard MutexGuard(mutex_);

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
            // Se usa rxbuffersize_ para el buffer de RX y TX, y se crea una cola de eventos
            ESP_ERROR_CHECK(uart_driver_install(port_, rxbuffersize_, 0, 20, &queue_, 0));

            // 4. Asignar memoria para el buffer de recepción temporal
            rxBuffer_ = (uint8_t *)malloc(rxbuffersize_);

            // 5. Crear la tarea de eventos
            if (rxBuffer_ != nullptr)
            {
                xTaskCreate(
                    eventTaskAdapter,  // Función de la tarea (el adaptador estático)
                    "uart_event_task", // Nombre de la tarea
                    2048,              // Tamaño del stack
                    this,              // Parámetro de la tarea (puntero a esta instancia)
                    10,                // Prioridad
                    &eventTaskHandle_  // Handle de la tarea
                );
            }
            else
            {
                FP_LOG_E(TAG, "Failed to allocate memory for RX buffer");
            }
        }

        void SimpleUart::close()
        {
            Core::Utils::MutexGuard MutexGuard(mutex_);

            // 1. Eliminar la tarea de eventos si existe
            if (eventTaskHandle_ != nullptr)
            {
                vTaskDelete(eventTaskHandle_);
                eventTaskHandle_ = nullptr;
            }

            // 2. Desinstalar el driver de la UART
            // Esto también libera la cola (queue_)
            uart_driver_delete(port_);

            // 3. Liberar el buffer de recepción
            if (rxBuffer_ != nullptr)
            {
                free(rxBuffer_);
                rxBuffer_ = nullptr;
            }

            // 4. Llamar al callback onClose si está definido
            if (onClose)
            {
                onClose();
            }
        }

        void SimpleUart::send(const uint8_t *data, size_t len)
        {
            Core::Utils::MutexGuard MutexGuard(mutex_);

            if (data == nullptr || len == 0)
            {
                return;
            }
            // Escribir datos en la UART
            uart_write_bytes(port_, (const char *)data, len);
        }

        void SimpleUart::eventTaskAdapter(void *arg)
        {
            // 1. Obtener la instancia
            SimpleUart *instance = static_cast<SimpleUart *>(arg);

            // 2. Llamar a la función miembro que contiene el bucle
            // El objeto se mantiene vivo por el shared_ptr original
            instance->eventTask();
        }

        void SimpleUart::eventTask()
        {
            if (onOpen)
            {
                onOpen();
            }
            uart_event_t event;
            for (;;) // Bucle infinito de la tarea
            {
                // Esperar por un evento en la cola
                if (xQueueReceive(queue_, (void *)&event, portMAX_DELAY))
                {
                    Core::Utils::MutexGuard MutexGuard(mutex_);

                    if (rxBuffer_ == nullptr)
                        break; // por si close() ha liberado los recursos mientras esperabamos

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
            eventTaskHandle_ = nullptr;
            vTaskDelete(NULL);
        }
    }
}