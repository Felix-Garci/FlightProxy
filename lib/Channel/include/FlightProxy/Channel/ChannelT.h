#pragma once

#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"

namespace FlightProxy
{
    namespace Channel
    {
        template <typename PacketT>
        class ChannelT : public Core::Channel::IChannelT<PacketT>
        {
        public:
            ChannelT(Core::Transport::ITransport *t, Core::Protocol::IEncoderT<PacketT> *e, Core::Protocol::IDecoderT<PacketT> *d)
                : transport_(t), encoder_(e), decoder_(d)
            {
                // 1. Flujo de ENTRADA: Transport -> Decoder
                transport_->onData = [this](const uint8_t *data, size_t len)
                {
                    if (decoder_)
                    {
                        decoder_->feed(data, len);
                    }
                };

                // 2. Flujo de ENTRADA: Decoder -> Channel
                decoder_->onPacket([this](const PacketT &packet) // <-- Llama a la función
                                   {
                    if (this->onPacket) 
                    { 
                        this->onPacket(packet);
                    } });

                // 3. Callbacks de estado: Transport -> Channel
                transport_->onOpen = [this]()
                {
                    if (this->onOpen)
                    {
                        this->onOpen();
                    }
                };

                transport_->onClose = [this]()
                {
                    if (this->onClose)
                    {
                        this->onClose();
                    }
                };
            }
            ~ChannelT() {}

            void open() override
            {
                transport_->open();
            }
            void close() override
            {
                transport_->close();
            }
            void sendPacket(const PacketT &packet) override
            {
                std::vector<uint8_t> encodedData = encoder_->encode(packet);
                transport_->send(encodedData.data(), encodedData.size());
            }

        private:
            Core::Transport::ITransport *transport_;
            Core::Protocol::IEncoderT<PacketT> *encoder_;
            Core::Protocol::IDecoderT<PacketT> *decoder_;
        };
    }
}