#pragma once
#include <cstdint>
#include <span>

namespace fcbridge::ibus
{

    // Interfaz mínima para portar a driver UART
    class UartPort
    {
    public:
        virtual ~UartPort() = default;
        virtual bool write(std::span<const uint8_t> bytes) = 0;
        virtual bool flush() = 0;
    };

} // namespace fcbridge::ibus
