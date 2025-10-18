#pragma once
#include "Structure.hpp"
#include <functional>

namespace tp::I
{
    class Interface
    {
    public:
        using CallBack = std::function<void(const tp::MSG::Frame &)>;

        virtual ~Interface() = default;
        virtual int write(const tp::MSG::Frame &frame) = 0;
        virtual void recive(const std::uint8_t *data, std::size_t len) = 0;
        virtual void setCallBack(CallBack c) = 0;
    };
}