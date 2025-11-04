#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include <memory>
#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace FlightProxy
{
    namespace Channel
    {
        static const char *TAG = "PersistentChannel";

        template <typename PacketT>
        class PersistentChannelT : public Core::Channel::IChannelT<PacketT>,
                                   public std::enable_shared_from_this<PersistentChannelT>
        {
        public:
            // --- Definición de Tipos (Factorías) ---
            using TransportPtr = std::shared_ptr<Core::Transport::ITransport>;
            using DecoderPtr = std::shared_ptr<Core::Protocol::IDecoderT<PacketT>>;
            using EncoderPtr = std::shared_ptr<Core::Protocol::IEncoderT<PacketT>>;

            using TransportFactory = std::function<TransportPtr()>;
            using DecoderFactory = std::function<DecoderPtr()>;
            using EncoderFactory = std::function<EncoderPtr()>;

            PersistentChannelT(TransportFactory tf, DecoderFactory df, EncoderFactory ef)
                : m_tf(tf), m_df(df), m_ef(ef), m_running(false), m_task_handle(nullptr),
                  m_mutex(xSemaphoreCreateRecursiveMutex())
            {
                if (!m_tf || !m_df || !m_ef)
                {
                    FP_LOG_E(TAG, "¡Las factorías no pueden ser nulas!");
                }
            }

            virtual ~PersistentChannelT()
            {
                // Paramos la tarea de reconexion
                close();

                // Esperar a que la tarea termine (Patrón "Join")
                int loops = 0;
                while (true)
                {
                    {
                        Core::Utils::MutexGuard lock(m_mutex);
                        if (m_task_handle == nullptr)
                            break; // La tarea ha muerto
                    }
                    vTaskDelay(pdMS_TO_TICKS(20));
                    if (++loops > 100) // Timeout de 2 segundos
                    {
                        FP_LOG_E(TAG, "Timeout esperando a la tarea. Forzando borrado.");
                        vTaskDelete(m_task_handle); // Matar a la fuerza
                        break;
                    }
                }
                if (m_mutex)
                    vSemaphoreDelete(m_mutex);
            }

            // --- Implementación de IChannelT (Métodos Públicos) ---

            void sendPacket(const PacketT &packet) override
            {
                Core::Utils::MutexGuard lock(m_mutex);
                if (m_internal_channel)
                {
                    // Pasa el paquete al canal interno
                    m_internal_channel->sendPacket(packet);
                }
                else
                {
                    FP_LOG_W(TAG, "Paquete descartado: Canal persistente no conectado.");
                }
            }

            void open() override
            {
                Core::Utils::MutexGuard lock(m_mutex);
                if (m_task_handle)
                    return; // Ya iniciada

                m_running = true;
                BaseType_t result = xTaskCreate(
                    reconnectionTaskAdapter,
                    "persistent_task", // Nombre
                    4096,              // Stack
                    this,              // Arg
                    5,                 // Prio
                    &m_task_handle);

                if (result != pdPASS)
                {
                    FP_LOG_E(TAG, "Error al crear la tarea de reconexión.");
                    m_task_handle = nullptr;
                    m_running = false;
                }
            }

            void close() override
            {
                Core::Utils::MutexGuard lock(m_mutex);
                if (!m_running)
                    return;

                m_running = false; // Avisa a la tarea que debe parar

                // Cierra el canal interno (si existe)
                // Esto provocará que su 'onClose' se llame y limpie m_internal_channel
                if (m_internal_channel)
                {
                    m_internal_channel->close();
                }
            }

            // Los callbacks (onPacket, onOpen, onClose) son heredados de IChannelT
            // y la App B se suscribirá a ellos.

        private:
            static void reconnectionTaskAdapter(void *arg)
            {
                PersistentChannelT *instance = static_cast<PersistentChannelT *>(arg);
                instance->reconnectionTask();
                vTaskDelete(NULL); // Auto-eliminarse
            }

            void reconnectionTask()
            {
                FP_LOG_I(TAG, "Tarea de reconexión iniciada.");

                while (m_running)
                {
                    // Si el canal no existe (y deberíamos estar corriendo)
                    if (m_internal_channel == nullptr && m_running)
                    {
                        FP_LOG_I(TAG, "Canal interno caído. Intentando (re)conectar...");
                        try
                        {
                            // --- 1. Crear componentes con las factorías ---
                            auto transport = m_tf();
                            auto decoder = m_df();
                            auto encoder = m_ef();

                            if (!transport || !decoder || !encoder)
                            {
                                throw std::runtime_error("Factorías devolvieron nullptr");
                            }

                            // --- 2. Crear el canal "núcleo" ---
                            auto newChannel = std::make_shared<ChannelT<PacketT>>(transport, encoder, decoder);

                            // --- 3. Conectar Callbacks ---

                            // Reenviar 'onPacket' y 'onOpen' a la App B
                            newChannel->onPacket = this->onPacket;
                            newChannel->onOpen = this->onOpen;

                            // *** ¡LA CLAVE! ***
                            // Interceptamos 'onClose'
                            newChannel->onClose = [this]()
                            {
                                FP_LOG_W(TAG, "onClose del canal interno detectado.");

                                // 1. Notificar a la App B (si está escuchando)
                                if (this->onClose)
                                {
                                    this->onClose();
                                }

                                // 2. Borrar el canal interno (esto disparará la reconexión)
                                Core::Utils::MutexGuard lock(m_mutex);
                                m_internal_channel.reset();
                            };

                            // --- 4. Abrir el canal (inicia su tarea y conexión) ---
                            newChannel->open();

                            // --- 5. Guardar el canal (thread-safe) ---
                            {
                                Core::Utils::MutexGuard lock(m_mutex);
                                m_internal_channel = newChannel;
                            }
                            FP_LOG_I(TAG, "Canal interno (re)conectado con éxito.");
                        }
                        catch (const std::exception &e)
                        {
                            FP_LOG_E(TAG, "Fallo al crear canal interno: %s", e.what());
                            Core::Utils::MutexGuard lock(m_mutex);
                            m_internal_channel.reset();
                        }
                    }

                    // Esperar 5s antes de volver a comprobar
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }

                // --- Limpieza de la Tarea ---
                FP_LOG_I(TAG, "Tarea de reconexión parada.");
                {
                    Core::Utils::MutexGuard lock(m_mutex);
                    m_internal_channel.reset(); // Asegurarse de que esté limpio
                    m_task_handle = nullptr;    // Avisar al destructor que hemos muerto
                }
            }

            // --- Variables Miembro ---
            std::shared_ptr<Core::Channel::IChannelT<PacketT>> m_internal_channel; // El canal "real"

            // Factorías ("Recetas")
            TransportFactory m_tf;
            DecoderFactory m_df;
            EncoderFactory m_ef;

            // Estado de la Tarea
            std::atomic<bool> m_running; // C++ atomic es seguro entre tareas
            TaskHandle_t m_task_handle;
            SemaphoreHandle_t m_mutex; // Protege m_internal_channel y m_task_handle
        };
    }
}
