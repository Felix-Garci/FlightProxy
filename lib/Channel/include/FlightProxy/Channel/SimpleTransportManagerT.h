#pragma once

#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy
{
    namespace Channel
    {
        const char *TAG = "SimpleTransportManagerT";

        template <typename PacketT>
        class SimpleTransportManagerT
        {
        public:
            // Para devolver el channel creado
            using ChannelCallback = std::function<void(Core::Channel::IChannelT<PacketT> *)>;
            ChannelCallback onNewChannel;

            // Recibiremos factories para encoder y decoder
            using DecoderFactory = std::function<Core::Protocol::IDecoderT<PacketT> *()>;
            using EncoderFactory = std::function<Core::Protocol::IEncoderT<PacketT> *()>;
            // Recibiremos factories para el transport
            using TransportFactory = std::function<Core::Transport::ITransport *()>;

            SimpleTransportManagerT()
                : packetChannel_(nullptr), transport_(nullptr), encoder_(nullptr), decoder_(nullptr)
            {
                FP_LOG_D(TAG, "Constructor llamado");
            }
            ~SimpleTransportManagerT()
            {
                delete packetChannel_;
                delete transport_;
                delete encoder_;
                delete decoder_;
            }

            void start(DecoderFactory df, EncoderFactory ef, TransportFactory tf)
            {
                FP_LOG_I(TAG, "Iniciando SimpleTransportManagerT...");
                // Liberar recursos antiguos si existen
                delete packetChannel_;
                delete decoder_;
                delete encoder_;
                delete transport_;

                // Ponerlos a nullptr por si algo falla
                packetChannel_ = nullptr;
                decoder_ = nullptr;
                encoder_ = nullptr;
                transport_ = nullptr;

                // Crear nuevos recursos

                encoder_ = ef();
                decoder_ = df();
                transport_ = tf();

                packetChannel_ = new ChannelT<PacketT>(transport_, encoder_, decoder_);

                if (onNewChannel)
                {
                    FP_LOG_I(TAG, "Ejecutando callback onNewChannel.");
                    onNewChannel(packetChannel_);
                }
                else
                {
                    FP_LOG_E(TAG, "onNewChannel callback not set!");
                }
            }

        private:
            ChannelT<PacketT> *packetChannel_;
            Core::Transport::ITransport *transport_;
            Core::Protocol::IEncoderT<PacketT> *encoder_;
            Core::Protocol::IDecoderT<PacketT> *decoder_;
        };
    }
}