#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace fcbridge::utils
{

    class MutexGuard
    {
    public:
        explicit MutexGuard(SemaphoreHandle_t mtx) : mtx_(mtx)
        {
            if (mtx_)
            {
                xSemaphoreTake(mtx_, portMAX_DELAY);
            }
        }

        ~MutexGuard()
        {
            if (mtx_)
            {
                xSemaphoreGive(mtx_);
            }
        }

        MutexGuard(const MutexGuard &) = delete;
        MutexGuard &operator=(const MutexGuard &) = delete;

    private:
        SemaphoreHandle_t mtx_ = nullptr;
    };

} // namespace fcbridge::utils
