#pragma once

#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include "FlightProxy/Channel/ChannelT.h"

#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy
{
    namespace Channel
    {
        const char *TAG = "UartTransportManagerT";

        template <typename PacketT>
        class UartTransportManagerT
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

            UartTransportManagerT()
                : packetChannel_(nullptr), transport_(nullptr), encoder_(nullptr), decoder_(nullptr)
            {
            }
            ~UartTransportManagerT()
            {
                delete packetChannel_;
                delete transport_;
                delete encoder_;
                delete decoder_;
            }

            void start(DecoderFactory df, EncoderFactory ef, TransportFactory tf)
            {
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