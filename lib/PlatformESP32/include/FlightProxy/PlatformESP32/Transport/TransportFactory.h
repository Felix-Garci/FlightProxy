#pragma once

#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Transport/ITcpListener.h"

#include "SimpleUart.h"
#include "SimpleUDP.h"
#include "SimpleTCP.h"
#include "ListenerTCP.h"

#include <memory>

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Transport
        {
            struct TransportFactory
            {
                static std::shared_ptr<Core::Transport::ITransport> CreateSimpleUart(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate)
                {
                    return std::make_shared<SimpleUart>(port, txpin, rxpin, baudrate);
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
