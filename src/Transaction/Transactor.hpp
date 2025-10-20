#pragma once
#include "freertos/semphr.h"
#include "utils/MutexGuard.hpp"
#include "Structure.hpp"

using tp::UTILS::MutexGuard;

namespace tp::T
{
    class Transactor
    {
    public:
        Transactor()
        {
            mtx_ = xSemaphoreCreateMutex();
        }
        ~Transactor()
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
            if (onFly_)
                return;
            onFly_ = true;
            if (downWriter_)
                downWriter_(in);
        }

        void SETdownwriter(TargetFunction downWriter)
        {
            downWriter_ = downWriter;
        }

        void ONReciveDOWN(tp::MSG::Frame &in)
        {
            MutexGuard lock(mtx_);
            if (!onFly_)
                return;

            if (upWriter_)
                upWriter_(in);
            onFly_ = false;
        }

        void SETupwriter(TargetFunction upWriter)
        {
            upWriter_ = upWriter;
        }

    private:
        mutable SemaphoreHandle_t mtx_ = nullptr;
        TargetFunction upWriter_;
        TargetFunction downWriter_;
        bool onFly_{false};
    };
}
