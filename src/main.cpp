// main.cpp
#include "hal/uart.hpp"
#include "hal/wifi.hpp"
#include "hal/tcp.hpp"
#include "utils/log.hpp"

extern "C" void app_main(void)
{
    SimpleUARTEvt uart;

    // Ajusta pines/puerto a tu hardware
    uart.begin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, 115200, 1024, 10);

    // Callback que se llama cada vez que llega data
    uart.onReceive([&](const uint8_t *data, size_t len)
                   {
        // Aquí estás en contexto de task (seguro para llamar API de IDF/FreeRTOS)
        uart.write(reinterpret_cast<const uint8_t *>("RX: "), 4);
        uart.write(data, len);
        uart.write("\r\n"); });

    WiFiSTA wifi;
    if (wifi.begin("Sup", "rrrrrrrr", 15000, 112) == ESP_OK)
    {
        uart.write("Wi-Fi connected!\r\n");
        uart.write(wifi.ipStr().c_str());
    }
    else
    {
        uart.write("Wi-Fi connection failed!\r\n");
        return;
    }

    SimpleTCPServer srv;

    // 1) Inicia el servidor en el puerto 12345
    if (srv.begin(12345) != ESP_OK)
    {
        printf("No se pudo iniciar el servidor\n");
        return;
    }

    // 2) Callback de recepción
    srv.onReceive([&](int sock, const uint8_t *data, size_t len)
                  {
                      // Echo: devuelve lo mismo
                      srv.write(data, len);
                      // Si quieres tratar por líneas:
                      // - acumula en un buffer propio hasta encontrar '\n'
                  });

    srv.start(4096, tskIDLE_PRIORITY + 2);

    // Tu app puede seguir haciendo otras cosas...
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*#include "net/TcpMspEndpoint.hpp"
#include "msp/MspSessionManager.hpp"
#include "msp/MspPassthroughProxy.hpp"
#include "msp/LocalCtrlHandler.hpp"
#include "rc/RcChannels.hpp"
#include "state/StateModel.hpp"
#include "ibus/IbusTransmitter.hpp"
#include "utils/log.hpp"

using namespace fcbridge;

int app_main(void)
{
    // Logs
    utils::Log::init("transponder");

    // Instancias
    rc::RcChannels rc;
    state::StateModel st;
    msp::LocalCtrlHandler ctrl;
    msp::MspSessionManager session;
    msp::MspPassthroughProxy proxy(&session);
    net::TcpMspEndpoint tcp(&session);
    ibus::IbusTransmitter ibus;

    // Wiring
    ctrl.setRc(&rc);
    ctrl.setState(&st);
    session.setCtrlHandler(&ctrl);
    session.setProxy(&proxy);
    session.setTcpEndpoint(&tcp);
    ibus.setChannelsSource(&rc);
    ibus.setStateModel(&st);

    // start() / UART/TCP setLowLevel... aquí

    return 0;
}
*/