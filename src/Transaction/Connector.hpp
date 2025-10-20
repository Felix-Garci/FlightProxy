#pragma once
#include "freertos/semphr.h"
#include "utils/MutexGuard.hpp"
#include "Structure.hpp"

using tp::UTILS::MutexGuard;

namespace tp::T
{
    class Connector
    {
    public:
        Connector()
        {
            mtx_ = xSemaphoreCreateMutex();
        }
        ~Connector()
        {
            if (mtx_)
            {
                vSemaphoreDelete(mtx_);
                mtx_ = nullptr;
            }
        }

        void ONReciveUP(tp::MSG::Frame &in) // Recepcion inicial -> entra en el callbak de la clase superior
        {
            MutexGuard lock(mtx_);
            if (downWriter_)
                downWriter_(in);
        }

        void SETdownwriter(TargetFunction downWriter)
        {
            downWriter_ = downWriter;
        }

    private:
        mutable SemaphoreHandle_t mtx_ = nullptr;
        TargetFunction downWriter_;
    };
}
