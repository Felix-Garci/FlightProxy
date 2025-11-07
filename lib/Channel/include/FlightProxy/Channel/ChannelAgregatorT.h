#pragma once
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"
#include <vector>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Channel
    {
        template <typename PacketT>
        class ChannelAgregatorT : public std::enable_shared_from_this<ChannelAgregatorT>
        {
        public:
            ChannelAgregatorT() : mutex_(xSemaphoreCreateMutex()) {}
            ~ChannelAgregatorT() { vSemaphoreDelete(mutex_); }

            void addChannel(std::shared_ptr<FlightProxy::Core::Channel::IChannelT<PacketT>> channel)
            {
                Core::Utils::MutexGuard lock(mutex_);
                channels_.push_back(channel);
                channel->onClose = [this, channel]()
                {
                    FP_LOG_I("MAIN", "Canal cerrado. Borrándolo de la lista.");
                    Core::Utils::MutexGuard lock(mutex_);
                    channels_.erase(std::remove(channels.begin(), channels.end(), channel), channels.end());
                };
                channel->onPacket = [this](const PacketT &packet)
                {
                    if (onPacketFromAnyChannel)
                        onPacketFromAnyChannel(packet);
                };
            }

            std::function<void(const PacketT &)> onPacketFromAnyChannel;

        private:
            SemaphoreHandle_t mutex_;
            std::vector<std::shared_ptr<FlightProxy::Core::Channel::IChannelT<PacketT>>> channels_;
        };

    }
}