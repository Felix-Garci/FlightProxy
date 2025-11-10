#include "FlightProxy/PlatformWin/Transport/SimpleUart.h"
#include "FlightProxy/Core/Utils/Logger.h"
#include <iostream>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace Transport
        {
            static const char *TAG = "SimpleUART_Mock";

            SimpleUart::SimpleUart(const std::string &portName, uint32_t baudRate)
            {
                FP_LOG_D(TAG, "Creado Mock UART (Puente para tests). Puerto ficticio: %s, Baud: %d", portName.c_str(), baudRate);
            }

            void SimpleUart::open()
            {
                std::lock_guard lock(mutex_);
                isOpen = true;
                if (onOpen)
                {
                    onOpen();
                }
                FP_LOG_D(TAG, "Canal abierto.");
            }

            void SimpleUart::close()
            {
                std::lock_guard lock(mutex_);
                isOpen = false;
                if (onClose)
                {
                    onClose();
                }
                FP_LOG_D(TAG, "Canal cerrado.");
            }

            void SimpleUart::send(const uint8_t *data, size_t len)
            {
                // 1. Registramos el envío internamente (bajo mutex para thread-safety)
                {
                    std::lock_guard lock(mutex_);
                    sendCount++;
                    lastSentData.assign(data, data + len);
                }

                FP_LOG_D(TAG, "Datos enviados (%zu bytes). Ejecutando loopback inmediato...", len);

                // 2. Simulación de RX (Loopback)
                // Devolvemos exactamente lo mismo que se envió.
                // Llamamos al callback SIN el mutex para evitar deadlocks si la app reacciona inmediatamente.
                if (onData)
                {
                    onData(data, len);
                }
            }

        } // namespace Transport
    }
} // namespace FlightProxy