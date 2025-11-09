#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include <mutex>
#include <map>
#include <atomic>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Channel
    {
        template <typename PacketT>
        class ChannelAgregatorT : public std::enable_shared_from_this<ChannelAgregatorT<PacketT>>
        {
        public:
            ChannelAgregatorT() : m_mutex(Core::OSAL::Factory::createMutex()) {}
            ~ChannelAgregatorT() {}

            void addChannel(std::shared_ptr<FlightProxy::Core::Channel::IChannelT<PacketT>> channel)
            {
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);

                // Id unico para cada canal nuevo
                uint32_t myId = nextChannelId_++;

                // Metemos en el map con el id
                channelsById_[myId] = channel;

                channel->onClose = [this, myId, channel]()
                {
                    FP_LOG_I("MAIN", "Canal cerrado. Borrándolo de la lista.");
                    std::lock_guard<Core::OSAL::IMutex> lock(*m_mutex);
                    channelsById_.erase(myId);
                };

                channel->onPacket = [this, myId](std::shared_ptr<const PacketT> packet)
                {
                    if (onPacketFromAnyChannel)
                    {
                        Core::PacketEnvelope<PacketT> envelope;
                        envelope.channelId = myId;
                        envelope.packet = packet;

                        onPacketFromAnyChannel(envelope);
                    }
                };
            }

            void response(uint32_t responseId, std::shared_ptr<const PacketT> packet)
            {
                channelsById_[responseId]->sendPacket(packet);
            }

            std::function<void(const Core::PacketEnvelope<PacketT> &)> onPacketFromAnyChannel;

        private:
            std::atomic<uint32_t> nextChannelId_{1};
            std::unique_ptr<Core::OSAL::IMutex> m_mutex;
            std::map<uint32_t, std::shared_ptr<FlightProxy::Core::Channel::IChannelT<PacketT>>> channelsById_;
        };

    }
}