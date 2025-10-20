#pragma once
#include "freertos/semphr.h"
#include "utils/MutexGuard.hpp"
#include "Structure.hpp"

using tp::UTILS::MutexGuard;

namespace tp::STORAGE
{
    template <typename SampleT>
    class Manager
    {
    public:
        Manager(const SampleT &def) : def_(def)
        {
            mtx_ = xSemaphoreCreateMutex();
        }

        ~Manager()
        {
            if (mtx_)
            {
                vSemaphoreDelete(mtx_);
                mtx_ = nullptr;
            }
        }

        void set(const SampleT &s)
        {
            MutexGuard lock(mtx_);
            current_ = s;
        }

        SampleT get(bool current = true)
        {
            MutexGuard lock(mtx_);
            if (current)
                return current_;
            else
                return def_;
        }

    private:
        mutable SemaphoreHandle_t mtx_ = nullptr;
        SampleT def_{};
        SampleT current_{};
    };
}
