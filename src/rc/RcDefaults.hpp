#pragma once
#include "RcTypes.hpp"

namespace fcbridge::rc::defaults
{
    inline RcSample stoped()
    {
        /*
        roll = 1500
        pitch = 1500
        yaw = 1500
        throttle = 1000
        */

        RcSample s{};
        for (auto &c : s.ch)
            c = rc::RcLimits().mid;

        // Reescrivimos el throtel a minimo
        s.ch[3] = rc::RcLimits().min;

        // Flags a 0 (default)
        s.flags = 0;
        return s;
    }

} // namespace fcbridge::rc::defaults
