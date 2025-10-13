// Wrapwe de ESP_LOG* para logs coherentes
#pragma once
#include <esp_log.h>
#include <string_view>

namespace fcbridge::utils
{
    class Log
    {
    public:
        static void init(std::string_view tag)
        {
            tag_ = tag;
            ESP_LOGI(tag_.data(), "Log initialized with tag: %s", tag_.data());
        }

        template <typename... Args>
        static void info(const char *format, Args... args)
        {
            ESP_LOGI(tag_.data(), format, args...);
        }

        template <typename... Args>
        static void warn(const char *format, Args... args)
        {
            ESP_LOGW(tag_.data(), format, args...);
        }

        template <typename... Args>
        static void error(const char *format, Args... args)
        {
            ESP_LOGE(tag_.data(), format, args...);
        }

    private:
        inline static std::string_view tag_ = "fcbridge"; // default tag
    };

} // namespace fcbridge::utils