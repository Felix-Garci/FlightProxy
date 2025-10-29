#include "SimpleTCP.h"

namespace FlightProxy
{
    namespace Transport
    {
        SimpleTCP::SimpleTCP(struct netconn *clientSocket)
            : clientSocket_(clientSocket), taskHandle_(nullptr)
        {
        }

        SimpleTCP::~SimpleTCP()
        {
            close();
        }

        void SimpleTCP::open()
        {
            // Implementation of TCP opening logic
        }

        void SimpleTCP::close()
        {
            // Implementation of TCP closing logic
        }

        void SimpleTCP::send(const uint8_t *data, size_t len)
        {
            // Implementation of TCP sending logic
        }

        void SimpleTCP::eventTask()
        {
            // Implementation of event task logic
        }

        void SimpleTCP::eventTaskAdapter(void *arg)
        {
            static_cast<SimpleTCP *>(arg)->eventTask();
        }
    }
}