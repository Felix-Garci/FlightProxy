#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <functional>
#include <memory>

namespace FlightProxy
{
    namespace Channel
    {
        const char *TAG = "SimpleTransportManagerT";

        template <typename PacketT>
        class SimpleTransportManagerT
        {
        public:
            // Tipos de punteros compartidos
            using ChannelPtr = std::shared_ptr<Core::Channel::IChannelT<PacketT>>;
            using DecoderPtr = std::shared_ptr<Core::Protocol::IDecoderT<PacketT>>;
            using EncoderPtr = std::shared_ptr<Core::Protocol::IEncoderT<PacketT>>;
            using TransportPtr = std::shared_ptr<Core::Transport::ITransport>;

            // Callbak de nuevo channel pasando un shared ptr
            using ChannelCallback = std::function<void(ChannelPtr)>;
            ChannelCallback onNewChannel;

            // Las factorias debuelven shared ptr
            using DecoderFactory = std::function<DecoderPtr()>;
            using EncoderFactory = std::function<EncoderPtr()>;
            using TransportFactory = std::function<TransportPtr()>;

            SimpleTransportManagerT()
            {
                FP_LOG_D(TAG, "Constructor llamado");
            }

            ~SimpleTransportManagerT()
            {
                FP_LOG_D(TAG, "Destructor llamado. Limpieza automática por shared_ptr.");
            }

            void start(DecoderFactory df, EncoderFactory ef, TransportFactory tf)
            {
                FP_LOG_I(TAG, "Iniciando SimpleTransportManagerT...");

                // Liberar recursos antiguos si existen
                packetChannel_.reset();
                decoder_.reset();
                encoder_.reset();
                transport_.reset();

                // Crear nuevos recursos
                encoder_ = ef();
                decoder_ = df();
                transport_ = tf();

                packetChannel_ = std::make_shared<ChannelT<PacketT>>(transport_, encoder_, decoder_);

                if (onNewChannel)
                {
                    FP_LOG_I(TAG, "Ejecutando callback onNewChannel.");
                    onNewChannel(packetChannel_); // Pasa el shared_ptr
                }
                else
                {
                    FP_LOG_E(TAG, "onNewChannel callback not set!");
                }
            }

        private:
            ChannelPtr packetChannel_;
            TransportPtr transport_;
            EncoderPtr encoder_;
            DecoderPtr decoder_;
        };
    }
}