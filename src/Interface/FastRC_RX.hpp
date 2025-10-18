#pragma once
#include "Interface.hpp"
#include "hal/udp.hpp"

namespace tp::I
{
    class FastRC_RX : public Interface
    {
    public:
        explicit FastRC_RX(SimpleUDPReceiver &u) : u_(u)
        {
            u_.onReceive([this](const std::uint8_t *data, std::size_t len, const sockaddr_in)
                         { recive(data, len); });
        }

        int write(const tp::MSG::Frame &frame)
        {
            (void)frame;
            return 0;
        }

        void recive(const std::uint8_t *data, std::size_t len)
        {
            tp::MSG::Frame frame;
            tp::MSG::decode(std::vector<std::uint8_t>(data, data + len), frame);
            if (cb_)
                cb_(frame);
        }
        void setCallBack(CallBack c)
        {
            cb_ = c;
        }

    private:
        SimpleUDPReceiver &u_;
        CallBack cb_;
    };
}