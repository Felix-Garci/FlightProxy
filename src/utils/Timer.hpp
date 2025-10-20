#pragma once
#include "Structure.hpp"
#include "esp_timer.h"
#include "esp_log.h"

namespace tp::UTILS
{
    class Timer
    {
    public:
        Timer() : running_(false), timer_handle_(nullptr) {}
        ~Timer()
        {
            stop();
            if (timer_handle_)
                esp_timer_delete(timer_handle_);
        }

        esp_err_t start(uint64_t period_ms, bool periodic)
        {
            uint64_t period_us = period_ms * 1000;
            stop(); // detener si ya corría

            esp_timer_create_args_t args = {
                .callback = &Timer::onTimeStatic,
                .arg = this,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "tp_utils_timer"};

            esp_err_t err = esp_timer_create(&args, &timer_handle_);
            if (err != ESP_OK)
                return err;

            running_ = true;

            if (periodic)
                err = esp_timer_start_periodic(timer_handle_, period_us);
            else
                err = esp_timer_start_once(timer_handle_, period_us);

            return err;
        }
        void stop()
        {
            if (running_ && timer_handle_)
            {
                esp_timer_stop(timer_handle_);
                esp_timer_delete(timer_handle_);
                timer_handle_ = nullptr;
                running_ = false;
            }
        }
        bool isRunning() const { return running_; }

        void setONTrigger(TargetFunction f) { callback_ = f; }

    private:
        static void onTimeStatic(void *arg)
        {
            Timer *self = static_cast<Timer *>(arg);
            if (self)
            {
                if (self->callback_)
                    self->callback_();
            }
        }
        bool running_;
        TargetFunction callback_;
        esp_timer_handle_t timer_handle_;
    };
}