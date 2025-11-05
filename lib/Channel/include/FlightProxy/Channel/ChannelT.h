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
            ChannelT(std::weak_ptr<Core::Transport::ITransport> t,
                     std::shared_ptr<Core::Protocol::IEncoderT<PacketT>> e,
                     std::shared_ptr<Core::Protocol::IDecoderT<PacketT>> d)
                : transport_(t), encoder_(e), decoder_(d) // Se copian los ptr
            {
                if (auto transport_ptr = transport_.lock())
                {
                    // El objeto existe, 'transport_ptr' es un std::shared_ptr válido
                    // transport_ptr->send(data, len);
                    transport_ptr->onData = [this](const uint8_t *data, size_t len)
                    {
                        if (decoder_)
                        {
                            decoder_->feed(data, len);
                        }
                    };
                    transport_ptr->onOpen = [this]()
                    {
                        if (this->onOpen)
                        {
                            this->onOpen();
                        }
                    };

                    transport_ptr->onClose = [this]()
                    {
                        if (this->onClose)
                        {
                            this->onClose();
                        }
                    };
                }

                decoder_->onPacket([this](const PacketT &packet)
                                   { 
                    if (this->onPacket) 
                    { 
                        this->onPacket(packet);
                    } });
            }

            ~ChannelT() {}

            void open() override
            {
                if (auto transport_ptr = transport_.lock())
                {
                    transport_ptr->open();
                }
            }

            void close() override
            {
                if (auto transport_ptr = transport_.lock())
                {
                    transport_ptr->close();
                }
            }

            void sendPacket(const PacketT &packet) override
            {
                if (auto transport_ptr = transport_.lock())
                {
                    std::vector<uint8_t> encodedData = encoder_->encode(packet);
                    transport_ptr->send(encodedData.data(), encodedData.size());
                }
            }

        private:
            std::weak_ptr<Core::Transport::ITransport> transport_;
            std::shared_ptr<Core::Protocol::IEncoderT<PacketT>> encoder_;
            std::shared_ptr<Core::Protocol::IDecoderT<PacketT>> decoder_;
        };
    }
}