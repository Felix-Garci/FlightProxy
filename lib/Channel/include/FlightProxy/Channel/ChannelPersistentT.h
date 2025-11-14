#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include <mutex>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Channel
    {
        static const char *TAG = "PersistentChannel";

        template <typename PacketT>
        class ChannelPersistentT : public Core::Channel::IChannelT<PacketT>,
                                   public std::enable_shared_from_this<ChannelPersistentT>
        {
        public:
            // --- Definición de Tipos (Factorías) ---
            using TransportPtr = std::shared_ptr<Core::Transport::ITransport>;
            using DecoderPtr = std::shared_ptr<Core::Protocol::IDecoderT<PacketT>>;
            using EncoderPtr = std::shared_ptr<Core::Protocol::IEncoderT<PacketT>>;

            using TransportFactory = std::function<TransportPtr()>;
            using DecoderFactory = std::function<DecoderPtr()>;
            using EncoderFactory = std::function<EncoderPtr()>;

            ChannelPersistentT(TransportFactory tf, DecoderFactory df, EncoderFactory ef)
                : m_tf(tf), m_df(df), m_ef(ef), m_running(false),
                  m_mutex(Core::OSAL::Factory::createMutex())
            {
                if (!m_tf || !m_df || !m_ef)
                {
                    FP_LOG_E(TAG, "¡Las factorías no pueden ser nulas!");
                }
            }

            virtual ~ChannelPersistentT()
            {
                // Paramos la tarea de reconexion
                close();
                m_reconnectTask->join();
            }

            // --- Implementación de IChannelT (Métodos Públicos) ---

            void sendPacket(const PacketT &packet) override
            {
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
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
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                if (m_running)
                    return; // Ya iniciada

                Core::OSAL::TaskConfig config;
                config.name = "PersistentTask";
                config.stackSize = 4096;
                config.priority = 5;

                // Usar OSAL Factory para crear la tarea
                m_reconnectTask = Core::OSAL::Factory::createTask(
                    [this]()
                    { this->reconnectionLoop(); },
                    config);

                if (m_reconnectTask)
                {
                    m_reconnectTask->start();
                }
                else
                {
                    FP_LOG_E(TAG, "Error al crear la tarea de reconexión.");
                    m_running = false;
                }
            }

            void close() override
            {
                // Primero señalizamos que pare
                m_running = false;

                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
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
                ChannelPersistentT *instance = static_cast<ChannelPersistentT *>(arg);
                instance->reconnectionTask();
                vTaskDelete(NULL); // Auto-eliminarse
            }

            void reconnectionTask()
            {
                FP_LOG_I(TAG, "Tarea de reconexión iniciada.");

                while (m_running)
                {
                    bool isConnected = false;
                    {
                        std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                        isConnected = (m_internal_channel != nullptr);
                    }

                    if (!isConnected && m_running)
                    {
                        FP_LOG_I(TAG, "Canal interno caído. Intentando (re)conectar...");
                        try
                        {
                            auto transport = m_tf();
                            auto decoder = m_df();
                            auto encoder = m_ef();

                            if (!transport || !decoder || !encoder)
                                throw std::runtime_error("Factorías devolvieron nullptr");

                            auto newChannel = std::make_shared<ChannelT<PacketT>>(transport, encoder, decoder);

                            newChannel->onPacket = this->onPacket;
                            newChannel->onOpen = this->onOpen;

                            // Usamos weak_ptr para evitar ciclo de referencias en la lambda
                            std::weak_ptr<ChannelPersistentT<PacketT>> weak_self = this->shared_from_this();
                            newChannel->onClose = [weak_self]()
                            {
                                if (auto self = weak_self.lock())
                                {
                                    FP_LOG_W(TAG, "onClose del canal interno detectado.");
                                    if (self->onClose)
                                        self->onClose();

                                    std::lock_guard<Core::OSAL::IMutex> lock(*self->m_mutex);
                                    self->m_internal_channel.reset();
                                }
                            };

                            newChannel->open();

                            {
                                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                                m_internal_channel = newChannel;
                            }
                            FP_LOG_I(TAG, "Canal interno (re)conectado con éxito.");
                        }
                        catch (const std::exception &e)
                        {
                            FP_LOG_E(TAG, "Fallo de reconexión: %s", e.what());
                            std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                            m_internal_channel.reset();
                        }
                    }

                    // Espera usando OSAL
                    if (m_running)
                    {
                        Core::OSAL::Factory::sleep(5000);
                    }
                }

                FP_LOG_I(TAG, "Tarea de reconexión parada.");
                {
                    std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                    m_internal_channel.reset();
                }
                // La tarea termina aquí y el wrapper OSAL se encarga de la limpieza
            }

            // --- Variables Miembro ---
            std::shared_ptr<Core::Channel::IChannelT<PacketT>> m_internal_channel; // El canal "real"

            // Factorías ("Recetas")
            TransportFactory m_tf;
            DecoderFactory m_df;
            EncoderFactory m_ef;

            // Estado de la Tarea
            std::atomic<bool> m_running; // C++ atomic es seguro entre tareas
            std::unique_ptr<Core::OSAL::ITask> m_reconnectTask;
            std::unique_ptr<Core::OSAL::IMutex> m_mutex;
        };
    }
}
