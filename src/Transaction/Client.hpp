#pragma once
#include "freertos/semphr.h"
#include "utils/MutexGuard.hpp"
#include "Structure.hpp"

using tp::UTILS::MutexGuard;

namespace tp::T
{
    class Client
    {
    public:
        Client()
        {
            mtx_ = xSemaphoreCreateMutex();
        }
        ~Client()
        {
            if (mtx_)
            {
                vSemaphoreDelete(mtx_);
                mtx_ = nullptr;
            }
        }

    private:
        mutable SemaphoreHandle_t mtx_ = nullptr;
    };
}
