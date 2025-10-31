#pragma once

#include "FlightProxy/Channel/PacketChannelT.h"
#include "FlightProxy/Channel/IPacketChannelT.h"
#include "FlightProxy/Transport/SimpleUart.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
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
            using ChannelCallback = std::function<void(IPacketChannelT<PacketT> *)>;
            ChannelCallback onNewChannel;

            // Recibiremos factories para encoder y decoder
            using DecoderFactory = std::function<Core::Protocol::IDecoderT<PacketT> *()>;
            using EncoderFactory = std::function<Core::Protocol::IEncoderT<PacketT> *()>;

            UartTransportManagerT(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate)
                : port_(port), txpin_(txpin), rxpin_(rxpin), baudrate_(baudrate),
                  packetChannel_(nullptr), transport_(nullptr), encoder_(nullptr), decoder_(nullptr)
            {
            }
            ~UartTransportManagerT()
            {
                delete packetChannel_;
                delete transport_;
                delete encoder_;
                delete decoder_;
            }

            void start(DecoderFactory df, EncoderFactory ef)
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
                transport_ = new FlightProxy::Transport::SimpleUart(port_, txpin_, rxpin_, baudrate_);

                encoder_ = ef();
                decoder_ = df();

                packetChannel_ = new PacketChannelT<PacketT>(transport_, encoder_, decoder_);

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
            uart_port_t port_;
            gpio_num_t txpin_;
            gpio_num_t rxpin_;
            uint32_t baudrate_;

            PacketChannelT<PacketT> *packetChannel_;
            Transport::SimpleUart *transport_;
            Core::Protocol::IEncoderT<PacketT> *encoder_;
            Core::Protocol::IDecoderT<PacketT> *decoder_;
        };
    }
}