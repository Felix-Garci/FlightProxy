// Wrapwe de ESP_LOG* para logs coherentes
#pragma once
#include <esp_log.h>
#include <string_view>
#include <cstdio>

namespace fcbridge::utils
{
    class Log
    {
    public:
        static void init(std::string_view tag)
        {
            tag_ = tag;
            // Asegura nivel global visible y deja rastro inicial
            esp_log_level_set("*", ESP_LOG_INFO);
            esp_log_write(ESP_LOG_INFO, tag_.data(), "Log initialized with tag: %s\n", tag_.data());
        }

        template <typename... Args>
        static void info(const char *format, Args &&...args)
        {
            char buf[256];
            int n = std::snprintf(buf, sizeof(buf), format, std::forward<Args>(args)...);
            (void)n; // ignorar truncado
            esp_log_write(ESP_LOG_INFO, tag_.data(), "%s\n", buf);
        }

        template <typename... Args>
        static void warn(const char *format, Args &&...args)
        {
            char buf[256];
            int n = std::snprintf(buf, sizeof(buf), format, std::forward<Args>(args)...);
            (void)n;
            esp_log_write(ESP_LOG_WARN, tag_.data(), "%s\n", buf);
        }

        template <typename... Args>
        static void error(const char *format, Args &&...args)
        {
            char buf[256];
            int n = std::snprintf(buf, sizeof(buf), format, std::forward<Args>(args)...);
            (void)n;
            esp_log_write(ESP_LOG_ERROR, tag_.data(), "%s\n", buf);
        }

    private:
        inline static std::string_view tag_ = "fcbridge"; // default tag
    };

} // namespace fcbridge::utils
