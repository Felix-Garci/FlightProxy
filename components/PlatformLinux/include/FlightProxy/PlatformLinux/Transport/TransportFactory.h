#pragma once

#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Transport/ITcpListener.h"

// Incluimos las versiones Linux
#include "FlightProxy/PlatformLinux/Transport/SimpleUart.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleUDP.h"
#include "FlightProxy/PlatformLinux/Transport/SimpleTCP.h"
#include "FlightProxy/PlatformLinux/Transport/ListenerTCP.h"

#include <memory>

namespace FlightProxy
{
    namespace PlatformLinux
    {
        namespace Transport
        {
            struct TransportFactory
            {
                static std::shared_ptr<Core::Transport::ITransport> CreateSimpleUart(const std::string &portName, uint32_t baudRate)
                {
                    return std::make_shared<SimpleUart>(portName, baudRate);
                }
                static std::shared_ptr<Core::Transport::ITransport> CreateSimpleUDP(uint16_t port)
                {
                    return std::make_shared<SimpleUDP>(port);
                }
                static std::shared_ptr<Core::Transport::ITransport> CreateSimpleTCP(const char *ip, uint16_t port)
                {
                    return std::make_shared<SimpleTCP>(ip, port);
                }
                static std::shared_ptr<Core::Transport::ITcpListener> CreateListenerTCP()
                {
                    return std::make_shared<ListenerTCP>();
                }
            };
        }
    }
}
