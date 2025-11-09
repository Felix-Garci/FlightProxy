#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Transport/ITcpListener.h"

#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include <mutex>
#include <functional>
#include <memory>
#include <vector>

namespace FlightProxy
{
    namespace Channel
    {
        static const char *TAG = "ChannelServer";

        template <typename PacketT>
        class ChannelServer
        {
        public:
            // --- Tipos de Punteros ---
            using ListenerPtr = std::shared_ptr<Core::Transport::ITcpListener>;

            using ChannelPtr = std::shared_ptr<Core::Channel::IChannelT<PacketT>>;

            using TransportPtr = std::weak_ptr<Core::Transport::ITransport>;
            using DecoderPtr = std::shared_ptr<Core::Protocol::IDecoderT<PacketT>>;
            using EncoderPtr = std::shared_ptr<Core::Protocol::IEncoderT<PacketT>>;

            // --- Tipos de Factorías (lo que recibimos) ---
            using ListenerFactory = std::function<ListenerPtr()>;
            using DecoderFactory = std::function<DecoderPtr()>;
            using EncoderFactory = std::function<EncoderPtr()>;

            // --- Callback de Salida (lo que producimos) ---
            using ChannelCallback = std::function<void(ChannelPtr)>;
            ChannelCallback onNewChannel;

            ChannelServer(DecoderFactory df, EncoderFactory ef, ListenerFactory lf)
                : m_decoderFactory(df), m_encoderFactory(ef), m_listenerFactory(lf),
                  m_mutex(Core::OSAL::Factory::createMutex())
            {
                if (!m_decoderFactory || !m_encoderFactory || !m_listenerFactory)
                {
                    FP_LOG_E(TAG, "¡Las factorías de Encoder/Decoder/Listener no pueden ser nulas!");
                }
            }

            ~ChannelServer()
            {
                stop();
            }

            bool start(uint16_t port)
            {
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);

                if (m_tcpListener)
                {
                    FP_LOG_W(TAG, "El listener TCP ya estaba iniciado.");
                    return true;
                }

                FP_LOG_I(TAG, "Iniciando listener TCP en puerto %u...", port);

                // 1. Crear el listener
                m_tcpListener = m_listenerFactory();

                // 2. Suscribirse al evento de nueva conexión
                m_tcpListener->onNewTransport = [this](TransportPtr newTransport)
                {
                    // Esto se ejecuta en la tarea del Listener
                    FP_LOG_I(TAG, "Cliente TCP aceptado. Construyendo canal...");

                    ChannelPtr newChannel = buildChannel(newTransport); // Construye y abre
                    if (newChannel)
                    {
                        // Notificamos a la aplicación.
                        // ¡¡NO lo guardamos!! La app es ahora la dueña.
                        if (onNewChannel)
                        {
                            onNewChannel(newChannel);
                        }
                        else
                        {
                            newChannel->close();
                        }
                    }
                };

                // 3. Iniciar el listener
                if (!m_tcpListener->startListening(port))
                {
                    FP_LOG_E(TAG, "¡Fallo al iniciar el listener TCP!");
                    m_tcpListener.reset(); // reset() libera el shared_ptr
                    return false;
                }
                return true;
            }

            void stop()
            {
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                FP_LOG_I(TAG, "Parando todos los servicios...");
                if (m_tcpListener)
                {
                    m_tcpListener->stopListening();
                    m_tcpListener.reset();
                }
            }

        private:
            ChannelPtr buildChannel(TransportPtr transport)
            {
                if (!m_decoderFactory || !m_encoderFactory)
                {
                    FP_LOG_E(TAG, "Fallo al construir canal: Factorías no configuradas.");
                    return nullptr;
                }

                auto decoder = m_decoderFactory();
                auto encoder = m_encoderFactory();

                if (!decoder || !encoder)
                {
                    FP_LOG_E(TAG, "Fallo al construir canal: Encoder/Decoder nulos.");
                    return nullptr;
                }
                if (auto transport_ptr = transport.lock())
                {
                    // 1. Crear el Channel (con shared_ptr)
                    auto newChannel = std::make_shared<ChannelT<PacketT>>(transport, encoder, decoder);

                    // 2. ¡¡Abrir el transporte!!
                    // Esto inicia la eventTask de SimpleTCP o SimpleUart
                    newChannel->open();
                    return newChannel;
                }
                else
                {
                    FP_LOG_E(TAG, "Fallo al construir canal: Transport nulo");
                    return nullptr;
                }
            }

            // --- Miembros Privados ---
            DecoderFactory m_decoderFactory;
            EncoderFactory m_encoderFactory;
            ListenerFactory m_listenerFactory;

            std::unique_ptr<Core::OSAL::IMutex> m_mutex;
            ListenerPtr m_tcpListener;
        };
    } // namespace Channel
} // namespace FlightProxy