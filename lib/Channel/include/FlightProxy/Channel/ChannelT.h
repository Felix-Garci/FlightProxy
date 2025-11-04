#pragma once

#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Protocol/IEncoderT.h"
#include "FlightProxy/Core/Protocol/IDecoderT.h"
#include <memory>

namespace FlightProxy
{
    namespace Channel
    {
        template <typename PacketT>
        class ChannelT : public Core::Channel::IChannelT<PacketT>
        {
        public:
            ChannelT(std::shared_ptr<Core::Transport::ITransport> t,
                     std::shared_ptr<Core::Protocol::IEncoderT<PacketT>> e,
                     std::shared_ptr<Core::Protocol::IDecoderT<PacketT>> d)
                : transport_(t), encoder_(e), decoder_(d) // Se copian los shared_ptr
            {

                transport_->onData = [this](const uint8_t *data, size_t len)
                {
                    if (decoder_)
                    {
                        decoder_->feed(data, len);
                    }
                };

                decoder_->onPacket([this](const PacketT &packet)
                                   { 
                    if (this->onPacket) 
                    { 
                        this->onPacket(packet);
                    } });

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
            std::shared_ptr<Core::Transport::ITransport> transport_;
            std::shared_ptr<Core::Protocol::IEncoderT<PacketT>> encoder_;
            std::shared_ptr<Core::Protocol::IDecoderT<PacketT>> decoder_;
        };
    }
}