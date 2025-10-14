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
        static void info(const char *format, Args &&...args)
        {
            esp_log_write(ESP_LOG_INFO, tag_.data(), format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void warn(const char *format, Args &&...args)
        {
            esp_log_write(ESP_LOG_WARN, tag_.data(), format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void error(const char *format, Args &&...args)
        {
            esp_log_write(ESP_LOG_ERROR, tag_.data(), format, std::forward<Args>(args)...);
        }

    private:
        inline static std::string_view tag_ = "fcbridge"; // default tag
    };

} // namespace fcbridge::utils