#include "FlightProxy/PlatformLinux/Transport/SimpleUart.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <iostream>

namespace FlightProxy
{
    namespace PlatformLinux
    {
        namespace Transport
        {
            static const char *TAG = "SimpleUART_Mock_Linux";

            SimpleUart::SimpleUart(const std::string &portName, uint32_t baudRate)
            {
                FP_LOG_D(TAG, "Creado Mock UART Linux. Puerto ficticio: %s, Baud: %d", portName.c_str(), baudRate);
            }

            void SimpleUart::open()
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                isOpen = true;
                if (onOpen)
                {
                    onOpen();
                }
                FP_LOG_D(TAG, "Canal abierto.");
            }

            void SimpleUart::close()
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                isOpen = false;
                if (onClose)
                {
                    onClose();
                }
                FP_LOG_D(TAG, "Canal cerrado.");
            }

            void SimpleUart::send(const uint8_t *data, size_t len)
            {
                // 1. Registramos envío
                {
                    std::lock_guard<std::recursive_mutex> lock(mutex_);
                    sendCount++;
                    lastSentData.assign(data, data + len);
                }

                FP_LOG_D(TAG, "Datos enviados (%zu bytes). Loopback inmediato...", len);

                // 2. Loopback
                if (onData)
                {
                    onData(data, len);
                }
            }

        } // namespace Transport
    }
} // namespace FlightProxy
