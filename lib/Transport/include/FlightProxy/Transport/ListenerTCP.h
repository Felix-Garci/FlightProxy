#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace FlightProxy
{
    namespace Transport
    {
        class ListenerTCP : public Core::Transport::ITcpListener
        {
        public:
            ListenerTCP();
            virtual ~ListenerTCP() override;

            bool startListening(uint16_t port) override;
            void stopListening() override;

        private:
            static void listener_task_entry(void *arg);

            TaskHandle_t m_listener_task_handle = NULL;
            int m_server_sock = -1;
        };

    } // namespace Transport
} // namespace FlightProxy