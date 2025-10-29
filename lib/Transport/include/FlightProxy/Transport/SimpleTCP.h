#pragma once
#include "FlightProxy/Transport/ITransport.h"
#include "freertos/task.h"
#include "lwip/api.h"

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleTCP : public ITransport
        {
        public:
            SimpleTCP(struct netconn *clientSocket);
            ~SimpleTCP() override;
            void open() override;
            void close() override;
            void send(const uint8_t *data, size_t len) override;

        private:
            netconn *clientSocket_;
            TaskHandle_t taskHandle_;
            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}
