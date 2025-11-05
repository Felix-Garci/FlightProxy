#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleTCP : public FlightProxy::Core::Transport::ITransport,
                          public std::enable_shared_from_this<SimpleTCP>
        {
        public:
            SimpleTCP(int accepted_socket);
            SimpleTCP(const char *ip, uint16_t port);
            ~SimpleTCP() override;

            void open() override;
            void close() override;
            void send(const uint8_t *data, size_t len) override;

        private:
            int m_sock = -1;
            uint16_t port_ = -1;
            char ip_[16];

            TaskHandle_t eventTaskHandle_;
            SemaphoreHandle_t mutex_;
            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}
