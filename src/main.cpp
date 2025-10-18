#include "net/TcpMspEndpoint.hpp"
#include "msp/MspSessionManager.hpp"
#include "msp/MspPassthroughProxy.hpp"
#include "msp/LocalCtrlHandler.hpp"
#include "rc/RcChannels.hpp"
#include "state/StateModel.hpp"
#include "ibus/IbusTransmitter.hpp"
#include "utils/Log.hpp"

#include "hal/wifi.hpp"
#include "hal/tcp.hpp"

#include <cstdint>
#include <cstddef>
#include <span>

using namespace fcbridge;

extern "C" void app_main(void)
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

    // WIFI
    WiFiSTA wifi;
    if (wifi.begin("Sup", "rrrrrrrr", 15000, 112) != ESP_OK)
        utils::Log::error("Wi-Fi connection failed");

    // TCP para recivir mensajes msp
    SimpleTCPServer srv;
    if (srv.begin(12345) != ESP_OK)
        utils::Log::error("TCP server start failed");

    // Este error es por el vscode, en vd no esta mal
    srv.onReceive([&](int, const uint8_t *data, std::size_t len)
                  { utils::Log::info("TCP RX %zu bytes", len);
                    tcp.onTcpBytes(std::span<const uint8_t>(data, len)); });

    tcp.setLowLevelSender([&](std::span<const uint8_t> bytes)
                          { utils::Log::info("TCP TX %zu bytes", bytes.size());
                            return srv.write(bytes.data(), bytes.size()) == bytes.size(); });

    srv.start(4096, tskIDLE_PRIORITY + 2);

    utils::Log::info("System initialized");

    for (;;)
        vTaskDelay(pdMS_TO_TICKS(1000));

    return;
}
