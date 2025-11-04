#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include <functional>
#include <memory>
#include <list>
namespace FlightProxy
{
    namespace Channel
    {
        static const char *TAG = "MultipleTransportManagerT";

        template <typename PacketT>
        class MultipleTransportManagerT
        {
        public:
            using ChannelCallback = std::function<void(Core::Channel::IChannelT<PacketT> *)>;
            using DecoderFactory = std::function<Core::Protocol::IDecoderT<PacketT> *()>;
            using EncoderFactory = std::function<Core::Protocol::IEncoderT<PacketT> *()>;

            ChannelCallback onNewChannel;

            MultipleTransportManagerT(
                std::shared_ptr<Core::Transport::ITcpListener> listener,
                DecoderFactory df,
                EncoderFactory ef)
                : m_listener(listener), m_decoderFactory(df), m_encoderFactory(ef)
            {
                FP_LOG_D(TAG, "Constructor llamado");

                // Creamos el mutex de FreeRTOS
                m_list_mutex = xSemaphoreCreateMutex();
                if (m_list_mutex == NULL)
                {
                    FP_LOG_E(TAG, "¡Error fatal al crear el mutex de la lista!");
                }

                if (!m_listener)
                {
                    FP_LOG_E(TAG, "¡Error! El ITcpListener es nulo.");
                }
                if (!m_decoderFactory || !m_encoderFactory)
                {
                    FP_LOG_E(TAG, "¡Error! Las fábricas de Encoder/Decoder son nulas.");
                }
            }

            ~MultipleTransportManagerT()
            {
                FP_LOG_D(TAG, "Destructor llamado");
                if (m_listener)
                {
                    m_listener->stopListening();
                }

                // Limpiamos el mutex de FreeRTOS
                if (m_list_mutex)
                {
                    vSemaphoreDelete(m_list_mutex);
                    m_list_mutex = NULL;
                }
                // m_channels se limpia automáticamente
            }

            /**
             * @brief Inicia el servidor.
             */
            void start(uint16_t port)
            {
                if (!m_listener || !m_decoderFactory || !m_encoderFactory || !m_list_mutex)
                {
                    FP_LOG_E(TAG, "No se puede iniciar, el manager no está configurado correctamente.");
                    return;
                }

                FP_LOG_I(TAG, "Iniciando MultipleTransportManagerT...");

                m_listener->onChannelAccepted = [this](std::shared_ptr<Core::Transport::ITransport> transport)
                {
                    this->onNewConnection(transport);
                };

                if (!m_listener->startListening(port))
                {
                    FP_LOG_E(TAG, "Error al iniciar el listener en el puerto %d", port);
                }
            }

        private:
            /**
             * @brief Callback de nueva conexión (ejecutado en la tarea del Listener)
             */
            void onNewConnection(std::shared_ptr<Core::Transport::ITransport> transport)
            {
                FP_LOG_I(TAG, "Nueva conexión aceptada. Creando canal...");

                auto managed = std::make_shared<ManagedChannel>();
                managed->transport = transport;
                managed->encoder.reset(m_encoderFactory());
                managed->decoder.reset(m_decoderFactory());

                managed->channel = std::make_shared<ChannelT<PacketT>>(
                    managed->transport.get(),
                    managed->encoder.get(),
                    managed->decoder.get());

                if (!managed->encoder || !managed->decoder || !managed->channel)
                {
                    FP_LOG_E(TAG, "Error al crear objetos del canal. Cerrando transporte.");
                    transport->close();
                    return;
                }

                managed->transport->onClose = [this, managed]()
                {
                    this->onConnectionClosed(managed);
                };

                // 5. Añadir a la lista usando tu MutexGuard
                {
                    // Usamos tu MutexGuard personalizado
                    Core::Utils::MutexGuard lock(m_list_mutex);
                    m_channels.push_back(managed);
                }

                if (onNewChannel)
                {
                    onNewChannel(managed->channel.get());
                }
                else
                {
                    FP_LOG_W(TAG, "onNewChannel callback no está configurado.");
                }
            }

            /**
             * @brief Callback de conexión cerrada (ejecutado en la tarea del SimpleTCP)
             */
            void onConnectionClosed(std::shared_ptr<ManagedChannel> channelToRemove)
            {
                FP_LOG_I(TAG, "Conexión cerrada. Eliminando canal.");

                // 1. Quitar de la lista usando tu MutexGuard
                {
                    // Usamos tu MutexGuard personalizado
                    Core::Utils::MutexGuard lock(m_list_mutex);
                    m_channels.remove(channelToRemove);
                }

                // 2. Limpieza automática por shared_ptr al salir del ámbito
            }

            // --- Miembros ---
            std::shared_ptr<Core::Transport::ITcpListener> m_listener;
            DecoderFactory m_decoderFactory;
            EncoderFactory m_encoderFactory;

            // Lista de canales activos
            std::list<std::shared_ptr<ManagedChannel>> m_channels;

            // Mutex de FreeRTOS para proteger la lista
            SemaphoreHandle_t m_list_mutex;

        private:
            // La estructura interna sigue igual, usa smart pointers
            struct ManagedChannel
            {
                std::shared_ptr<Core::Transport::ITransport> transport;
                std::shared_ptr<Core::Protocol::IEncoderT<PacketT>> encoder;
                std::shared_ptr<Core::Protocol::IDecoderT<PacketT>> decoder;
                std::shared_ptr<ChannelT<PacketT>> channel;
            };
        };
    } // namespace Channel
} // namespace FlightProxy