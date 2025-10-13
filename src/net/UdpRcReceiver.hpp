#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include "rc/RcTypes.hpp"

namespace fcbridge::rc
{
    class RcChannels;
}

namespace fcbridge::state
{
    class StateModel;
}

namespace fcbridge::net
{

    struct UdpParams
    {
        uint16_t listenPort = 14550;
        uint16_t packetSize = 32; // tamaño esperado RC
    };

    class UdpRcReceiver
    {
    public:
        UdpRcReceiver() = default;

        void configure(const UdpParams &p);
        void setChannelsTarget(rc::RcChannels *target);
        void setStateModel(state::StateModel *state);

        bool start();
        void stop();
        bool isRunning() const { return running_; }

    private:
        void recvLoop();

        UdpParams params_{};
        rc::RcChannels *rc_ = nullptr;
        state::StateModel *state_ = nullptr;

        std::atomic<bool> running_{false};
    };

} // namespace fcbridge::net
