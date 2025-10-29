#include "SimpleUart.h"

namespace FlightProxy
{
    namespace Transport
    {
        SimpleUart::SimpleUart(uart_port_t port, gpio_num_t txpin, gpio_num_t rxpin, uint32_t baudrate)
            : port_(port), txpin_(txpin), rxpin_(rxpin), baudrate_(baudrate), queue_(nullptr),
              eventTaskHandle_(nullptr), rxBuffer_(nullptr), rxbuffersize_(1024)
        {
                }

        SimpleUart::~SimpleUart()
        {
            close();
        }

        void SimpleUart::open()
        {
            // Implementation of UART opening logic
        }

        void SimpleUart::close()
        {
            // Implementation of UART closing logic
        }

        void SimpleUart::send(const uint8_t *data, size_t len)
        {
            // Implementation of UART sending logic
        }

        void SimpleUart::eventTask()
        {
            // Implementation of event task logic
        }

        void SimpleUart::eventTaskAdapter(void *arg)
        {
            static_cast<SimpleUart *>(arg)->eventTask();
        }
    }
}