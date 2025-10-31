#pragma once
#include "FlightProxy/Core/Transport/ITransport.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/api.h"

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleTCP : public FlightProxy::Core::Transport::ITransport
        {
        public:
            SimpleTCP(struct netconn *clientSocket);
            ~SimpleTCP() override;
            void open() override;
            void close() override;
            void send(const uint8_t *data, size_t len) override;

        private:
            netconn *clientSocket_;
            TaskHandle_t eventTaskHandle_;
            SemaphoreHandle_t mutex_;
            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}
