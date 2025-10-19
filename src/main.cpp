#include <cstdint>
#include <cstddef>
#include <span>

#include "Structure.hpp"
#include "utils/Log.hpp"

#include "hal/wifi.hpp"
#include "hal/tcp.hpp"
#include "hal/udp.hpp"
#include "hal/uart.hpp"

#include "Interface/Client.hpp"
#include "Interface/FC.hpp"
#include "Interface/FastRC_RX.hpp"
#include "Interface/FastRC_TX.hpp"

#include "Storage/Manager.hpp"

extern "C" void app_main(void)
{
    // Logs
    tp::UTILS::Log::init("transponder");

    // HAL Creation
    // WIFI
    tp::HAL::WiFiSTA wifi;
    if (wifi.begin("Sup", "rrrrrrrr", 15000, 112) != ESP_OK)
    {
        tp::UTILS::Log::error("Wi-Fi connection failed");
    }

    // TCP para recivir mensajes msp
    tp::HAL::SimpleTCPServer srv;
    if (srv.begin(12345) != ESP_OK)
    {
        tp::UTILS::Log::error("TCP server start failed");
    }
    // INTERFAZ
    tp::I::Client IClient(srv);
    srv.onReceive([&](int, const uint8_t *data, std::size_t len)
                  { IClient.recive(data, len); });

    // Uart A para MSP FC
    tp::HAL::SimpleUARTEvt uartA;
    if (uartA.begin(UART_NUM_1, GPIO_NUM_18, GPIO_NUM_19, 115200, 1024, 10) != ESP_OK)
    {
        tp::UTILS::Log::error("UARTA start failed");
    }
    // INTERFAZ
    tp::I::FC IFC(uartA);

    // UDP para recivir RC
    tp::HAL::SimpleUDPReceiver udp;
    if (udp.begin(5005) != ESP_OK)
    {
        tp::UTILS::Log::error("UDP start failed");
    }
    // INTERFAZ
    tp::I::FastRC_RX IFastRecive(udp);
    udp.onReceive([&](const uint8_t *data, size_t len,
                      const sockaddr_in)
                  { IFastRecive.recive(data, len); });

    // Uart B para IBus FC
    tp::HAL::SimpleUARTEvt uartB;
    if (uartB.begin(UART_NUM_2, GPIO_NUM_20, GPIO_NUM_21, 115200, 1024, 10) != ESP_OK)
    {
        tp::UTILS::Log::error("UARTB start failed");
    }
    // INTERFAZ
    tp::I::FastRC_TX IFastSender(uartB);

    // Storages
    tp::STORAGE::FastRCStatusSample FastRCStatus_def{false, 7};
    tp::STORAGE::Manager FastRCStatusManager{FastRCStatus_def};

    tp::STORAGE::RCSample RCSample_def = {{1500, 1500, 1500, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};
    tp::STORAGE::Manager RCSampleManager{RCSample_def};

    // Arrancamos servidor TCP
    srv.start(4096, tskIDLE_PRIORITY + 2);

    tp::UTILS::Log::info("System initialized");

    for (;;)
        vTaskDelay(pdMS_TO_TICKS(1000));

    return;
}
