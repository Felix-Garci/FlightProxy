#pragma once
#include "lwip/api.h"
#include "freertos/FreeRTOS.h"

/*
class SimpleTCP{
    -clientSocket_: netconn *
    -taskHandle_: TaskHandle_t

    ' Métodos
    +SimpleTCP(clientSocket: netconn *)
    +~SimpleTCP()
    +open(): void
    +close(): void
    +send(data:const uint8_t*,len:size_t): void
    -eventTask(): void
    -eventTaskAdapter(arg:void*): void
}
*/

namespace FlightProxy
{
    namespace Transport
    {
        class SimpleTCP
        {
        public:
            SimpleTCP(struct netconn *clientSocket);
            ~SimpleTCP();
            void open();
            void close();
            void send(const uint8_t *data, size_t len);

        private:
            struct netconn *clientSocket_;
            TaskHandle_t taskHandle_;

            void eventTask();
            static void eventTaskAdapter(void *arg);
        };
    }
}
