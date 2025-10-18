#pragma once
#include "Interface.hpp"
#include "hal/uart.hpp"

namespace tp::I
{
    class FastRC_TX : public Interface
    {
    public:
        explicit FastRC_TX(tp::HAL::SimpleUARTEvt &u) : u_(u)
        {
            u_.onReceive([this](const std::uint8_t *data, std::size_t len)
                         { recive(data, len); });
        }

        int write(const tp::MSG::Frame &frame)
        {
            std::vector<uint8_t> out;
            tp::MSG::encode(frame, out);
            return u_.write(out.data(), out.size());
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
        tp::HAL::SimpleUARTEvt &u_;
        CallBack cb_;
    };
}