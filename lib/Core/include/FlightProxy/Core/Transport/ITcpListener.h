#pragma once

#include "ITransport.h"
#include <memory>
#include <functional>
#include <cstdint>

namespace FlightProxy
{
    namespace Core
    {
        namespace Transport
        {
            class ITcpListener
            {
            public:
                virtual ~ITcpListener() = default;

                virtual bool startListening(uint16_t port) = 0;
                virtual void stopListening() = 0;

                // Callbak con el file descriptor del soket creado al recivir a un nuevo cliente
                std::function<void(int)> onChannelAccepted;
            };

        } // namespace Transport
    } // namespace Core
} // namespace FlightProxy