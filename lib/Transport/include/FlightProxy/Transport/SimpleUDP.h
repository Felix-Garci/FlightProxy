#pragma once
#include "FlightProxy/Transport/ITransport.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleUDP : public ITransport
        {
        public:
            SimpleUDP(uint16_t localPort);
            ~SimpleUDP() override;

            void open() override;
            void close() override;
            void send(const uint8_t *data, size_t size) override;

        private:
            netconn *udpSocket_;
            TaskHandle_t taskHandle_;
            uint16_t localPort_;
            ip_addr_t remoteIP_;
            uint16_t remotePort_;
            SemaphoreHandle_t remoteMutex_;
            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}