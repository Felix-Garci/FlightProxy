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

    // start() / UART/TCP setLowLevel... aquí

    // WIFI
    WiFiSTA wifi;
    if (wifi.begin("Sup", "rrrrrrrr", 15000, 112) != ESP_OK)
        return;

    // TCP para recivir mensajes msp
    SimpleTCPServer srv;
    if (srv.begin(12345) != ESP_OK)
        return;

    srv.onReceive([&](int, const uint8_t *data, std::size_t len)
                  { tcp.onTcpBytes(std::span<const uint8_t>(data, len)); });

    tcp.setLowLevelSender([&](std::span<const uint8_t> bytes)
                          { return srv.write(bytes.data(), bytes.size()) == bytes.size(); });

    srv.start(4096, tskIDLE_PRIORITY + 2);

    return;
}
