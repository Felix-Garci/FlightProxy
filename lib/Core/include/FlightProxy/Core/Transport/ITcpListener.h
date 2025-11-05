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

                using TransportCallback = std::function<void(std::weak_ptr<ITransport>)>;
                TransportCallback onNewTransport;

                virtual bool startListening(uint16_t port) = 0;
                virtual void stopListening() = 0;
            };

        } // namespace Transport
    } // namespace Core
} // namespace FlightProxy