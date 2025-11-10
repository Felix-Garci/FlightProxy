#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <memory>
#include <mutex>

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace Transport
        {
            class ListenerTCP : public Core::Transport::ITcpListener,
                                public std::enable_shared_from_this<ListenerTCP>
            {
            public:
                ListenerTCP();
                virtual ~ListenerTCP() override;

                bool startListening(uint16_t port) override;
                void stopListening() override;

            private:
                void listenerTask();
                static void listenerTaskAdapter(void *arg);

                TaskHandle_t m_listener_task_handle = NULL;
                int m_server_sock = -1;
                std::unique_ptr<Core::OSAL::IMutex> m_mutex;
            };

        } // namespace Transport
    }
} // namespace FlightProxy