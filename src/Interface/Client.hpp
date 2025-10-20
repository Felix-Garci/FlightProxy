#pragma once
#include "Interface.hpp"
#include "hal/tcp.hpp"
#include "utils/Log.hpp"

namespace tp::I
{
    class Client : public Interface
    {
    public:
        explicit Client(tp::HAL::SimpleTCPServer &s) : srv_(s)
        {
            srv_.onReceive([this](int, const std::uint8_t *data, std::size_t len)
                           { recive(data, len); });
        }

        int write(const tp::MSG::Frame &frame)
        {
            std::vector<uint8_t> out;
            tp::MSG::encode(frame, out);
            return srv_.write(out.data(), out.size());
        }

        void recive(const std::uint8_t *data, std::size_t len)
        {
            tp::MSG::Frame frame;
            bool ok = tp::MSG::decode(std::vector<std::uint8_t>(data, data + len), frame);
            if (!ok)
            {
                tp::UTILS::Log::warn("Client decode failed: %u bytes", (unsigned)len);
                return;
            }
            if (cb_)
                cb_(frame);
        }

        void setCallBack(CallBack c)
        {
            cb_ = c;
        }

    private:
        tp::HAL::SimpleTCPServer &srv_;
        CallBack cb_;
    };
}
